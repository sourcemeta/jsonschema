#include "http.h"

// NSURL, NSMutableURLRequest, NSURLSession, NSHTTPURLResponse, dispatch_*
#import <Foundation/Foundation.h>

#include <sourcemeta/core/text.h>

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

namespace sourcemeta::jsonschema {

auto http_request(const HTTPRequest &request) -> HTTPResponse {
  HTTPResponse response;
  // The completion handler runs on a background queue, where throwing
  // would terminate the process, so failures are recorded here and
  // thrown from the calling thread once the request settles
  std::string failure;

  @autoreleasepool {
    NSURL *target{[NSURL URLWithString:to_nsstring(request.url)]};
    if (target == nil) {
      failure = "Invalid URL";
    } else {
      NSMutableURLRequest *url_request{
          [NSMutableURLRequest requestWithURL:target]};
      url_request.HTTPMethod = to_nsstring(http_method_string(request.method));
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

      NSURLSession *session{[NSURLSession
          sessionWithConfiguration:[NSURLSessionConfiguration
                                       ephemeralSessionConfiguration]]};
      dispatch_semaphore_t semaphore{dispatch_semaphore_create(0)};
      // The completion handler runs before the semaphore is signalled, so
      // pointing to the stack-allocated locals from the block is safe
      HTTPResponse *response_pointer{&response};
      std::string *failure_pointer{&failure};
      const auto maximum_response_size{request.maximum_response_size};
      NSURLSessionDataTask *task{[session
          dataTaskWithRequest:url_request
            completionHandler:^(NSData *data, NSURLResponse *raw_response,
                                NSError *error) {
              if (error != nil) {
                failure_pointer->assign(
                    [error.localizedDescription UTF8String]);
              } else if (![raw_response
                             isKindOfClass:[NSHTTPURLResponse class]]) {
                failure_pointer->assign("The response is not an HTTP response");
              } else if (maximum_response_size.has_value() && data != nil &&
                         data.length > maximum_response_size.value()) {
                failure_pointer->assign(HTTP_RESPONSE_TOO_LARGE_MESSAGE);
              } else {
                const auto *http_response{(NSHTTPURLResponse *)raw_response};
                response_pointer->status = http_status_from_code(
                    static_cast<std::uint16_t>(http_response.statusCode));
                [http_response.allHeaderFields
                    enumerateKeysAndObjectsUsingBlock:^(
                        NSString *name, NSString *value, BOOL *) {
                      std::string header_name{[name UTF8String]};
                      sourcemeta::core::to_lowercase(header_name);
                      response_pointer->headers.emplace_back(
                          std::move(header_name), [value UTF8String]);
                    }];
                if (data != nil) {
                  response_pointer->body.assign(
                      static_cast<const char *>(data.bytes), data.length);
                }
              }

              dispatch_semaphore_signal(semaphore);
            }]};
      [task resume];
      dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
      [session finishTasksAndInvalidate];
    }
  }

  if (!failure.empty()) {
    throw HTTPError{request.method, std::string{request.url}, failure};
  }

  return response;
}

} // namespace sourcemeta::jsonschema
