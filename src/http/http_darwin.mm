#include "http.h"

// NSURL, NSMutableURLRequest, NSURLSession, NSHTTPURLResponse, dispatch_*
#import <Foundation/Foundation.h>

#include <sourcemeta/core/text.h>

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint16_t
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace {

auto to_nsstring(const std::string_view input) -> NSString * {
  return [[NSString alloc] initWithBytes:input.data()
                                  length:input.size()
                                encoding:NSUTF8StringEncoding];
}

} // namespace

// The delegate-based API streams the response body in chunks, allowing
// the maximum response size to be enforced without first buffering the
// entire response in memory
@interface SourcemetaJSONSchemaHTTPDelegate
    : NSObject <NSURLSessionDataDelegate>
@property(nonatomic, assign) sourcemeta::jsonschema::HTTPResponse *response;
@property(nonatomic, assign) std::string *failure;
@property(nonatomic, strong) dispatch_semaphore_t semaphore;
@property(nonatomic, assign) BOOL hasMaximumResponseSize;
@property(nonatomic, assign) std::size_t maximumResponseSize;
@end

@implementation SourcemetaJSONSchemaHTTPDelegate

- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveData:(NSData *)data {
  auto *body{&self.response->body};
  if (self.hasMaximumResponseSize &&
      body->size() + data.length > self.maximumResponseSize) {
    self.failure->assign(
        sourcemeta::jsonschema::HTTP_RESPONSE_TOO_LARGE_MESSAGE);
    [dataTask cancel];
    return;
  }

  [data enumerateByteRangesUsingBlock:^(const void *bytes, NSRange range,
                                        BOOL *) {
    body->append(static_cast<const char *>(bytes), range.length);
  }];
}

- (void)URLSession:(NSURLSession *)session
                    task:(NSURLSessionTask *)task
    didCompleteWithError:(NSError *)error {
  // A failure recorded while streaming, such as exceeding the maximum
  // response size, takes precedence over the resulting cancellation error
  if (self.failure->empty()) {
    if (error != nil) {
      self.failure->assign([error.localizedDescription UTF8String]);
    } else if (![task.response isKindOfClass:[NSHTTPURLResponse class]]) {
      self.failure->assign("The response is not an HTTP response");
    } else {
      const auto *http_response{(NSHTTPURLResponse *)task.response};
      self.response->status = sourcemeta::core::http_status_from_code(
          static_cast<std::uint16_t>(http_response.statusCode));
      auto *headers{&self.response->headers};
      [http_response.allHeaderFields
          enumerateKeysAndObjectsUsingBlock:^(NSString *name, NSString *value,
                                              BOOL *) {
            std::string header_name{[name UTF8String]};
            sourcemeta::core::to_lowercase(header_name);
            headers->emplace_back(std::move(header_name), [value UTF8String]);
          }];
    }
  }

  dispatch_semaphore_signal(self.semaphore);
}

@end

namespace sourcemeta::jsonschema {

auto http_request(const HTTPRequest &request) -> HTTPResponse {
  HTTPResponse response;
  // The delegate runs on a background queue, where throwing would
  // terminate the process, so failures are recorded here and thrown
  // from the calling thread once the request settles
  std::string failure;

  @autoreleasepool {
    NSURL *target{[NSURL URLWithString:to_nsstring(request.url)]};
    if (target == nil) {
      failure = "Invalid URL";
    } else {
      NSMutableURLRequest *url_request{
          [NSMutableURLRequest requestWithURL:target]};
      url_request.HTTPMethod =
          to_nsstring(sourcemeta::core::http_method_string(request.method));
      for (const auto &[name, value] : request.headers) {
        // Repeated headers are folded into a single comma-separated field
        // line, which is semantically equivalent per RFC 9110
        [url_request addValue:to_nsstring(value)
            forHTTPHeaderField:to_nsstring(name)];
      }

      if (request.body.has_value()) {
        [url_request setValue:to_nsstring(request.body.value().content_type)
            forHTTPHeaderField:@"Content-Type"];
        url_request.HTTPBody =
            [NSData dataWithBytes:request.body.value().data.data()
                           length:request.body.value().data.size()];
      }

      // The delegate completes before the semaphore is signalled, so
      // pointing to the stack-allocated locals from it is safe
      SourcemetaJSONSchemaHTTPDelegate *delegate{
          [[SourcemetaJSONSchemaHTTPDelegate alloc] init]};
      delegate.response = &response;
      delegate.failure = &failure;
      delegate.semaphore = dispatch_semaphore_create(0);
      delegate.hasMaximumResponseSize =
          request.maximum_response_size.has_value() ? YES : NO;
      delegate.maximumResponseSize = request.maximum_response_size.value_or(0);

      NSURLSession *session{[NSURLSession
          sessionWithConfiguration:[NSURLSessionConfiguration
                                       ephemeralSessionConfiguration]
                          delegate:delegate
                     delegateQueue:nil]};
      NSURLSessionDataTask *task{[session dataTaskWithRequest:url_request]};
      [task resume];
      dispatch_semaphore_wait(delegate.semaphore, DISPATCH_TIME_FOREVER);
      [session finishTasksAndInvalidate];
    }
  }

  if (!failure.empty()) {
    throw sourcemeta::core::HTTPError{request.method, std::string{request.url},
                                      failure};
  }

  return response;
}

} // namespace sourcemeta::jsonschema
