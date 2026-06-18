#include <sourcemeta/core/http.h>
#include <sourcemeta/core/text.h>

// NSURL, NSMutableURLRequest, NSURLSession, NSHTTPURLResponse, dispatch_*
#import <Foundation/Foundation.h>

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint16_t
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace {

constexpr std::string_view HTTP_RESPONSE_TOO_LARGE_MESSAGE{
    "The response exceeds the maximum allowed size"};

auto to_nsstring(const std::string_view input) -> NSString * {
  return [[NSString alloc] initWithBytes:input.data()
                                  length:input.size()
                                encoding:NSUTF8StringEncoding];
}

} // namespace

// The delegate-based API streams the response body in chunks, allowing
// the maximum response size to be enforced without first buffering the
// entire response in memory
@interface SourcemetaCoreHTTPDelegate : NSObject <NSURLSessionDataDelegate>
@property(nonatomic, assign) sourcemeta::core::HTTPResponse *response;
@property(nonatomic, assign) std::string *failure;
@property(nonatomic, strong) dispatch_semaphore_t semaphore;
@property(nonatomic, assign) BOOL hasMaximumResponseSize;
@property(nonatomic, assign) std::size_t maximumResponseSize;
@property(nonatomic, assign) BOOL followRedirects;
@property(nonatomic, assign) std::size_t maximumRedirects;
@property(nonatomic, assign) std::size_t redirectCount;
@end

@implementation SourcemetaCoreHTTPDelegate

- (void)URLSession:(NSURLSession *)session
                          task:(NSURLSessionTask *)task
    willPerformHTTPRedirection:(NSHTTPURLResponse *)response
                    newRequest:(NSURLRequest *)request
             completionHandler:
                 (void (^)(NSURLRequest *))completionHandler {
  // Passing a nil request stops the redirection and delivers the redirect
  // response itself as the final response
  if (!self.followRedirects) {
    completionHandler(nil);
    return;
  }

  self.redirectCount += 1;
  if (self.redirectCount > self.maximumRedirects) {
    self.failure->assign("The maximum number of redirects was exceeded");
    [task cancel];
    completionHandler(nil);
    return;
  }

  completionHandler(request);
}

- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveData:(NSData *)data {
  auto *body{&self.response->body};
  if (self.hasMaximumResponseSize &&
      (body->size() > self.maximumResponseSize ||
       static_cast<std::size_t>(data.length) >
           self.maximumResponseSize - body->size())) {
    self.failure->assign(HTTP_RESPONSE_TOO_LARGE_MESSAGE);
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
      if (http_response.URL != nil) {
        self.response->url.assign([http_response.URL.absoluteString UTF8String]);
      }

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

namespace sourcemeta::core {

auto HTTPSystemRequest::send() const -> HTTPResponse {
  HTTPResponse response;
  // The delegate runs on a background queue, where throwing would
  // terminate the process, so failures are recorded here and thrown
  // from the calling thread once the request settles
  std::string failure;

  @autoreleasepool {
    NSURL *target{[NSURL URLWithString:to_nsstring(this->url_)]};
    if (target == nil) {
      failure = "Invalid URL";
    } else {
      NSMutableURLRequest *url_request{
          [NSMutableURLRequest requestWithURL:target]};
      url_request.HTTPMethod = to_nsstring(http_method_string(this->method_));
      for (const auto &[name, value] : this->headers_) {
        // Repeated headers are folded into a single comma-separated field
        // line, which is semantically equivalent per RFC 9110
        [url_request addValue:to_nsstring(value)
            forHTTPHeaderField:to_nsstring(name)];
      }

      if (this->body_.has_value()) {
        [url_request setValue:to_nsstring(this->body_.value().content_type)
            forHTTPHeaderField:@"Content-Type"];
        url_request.HTTPBody =
            [NSData dataWithBytes:this->body_.value().data.data()
                           length:this->body_.value().data.size()];
      }

      NSURLSessionConfiguration *configuration{
          [NSURLSessionConfiguration ephemeralSessionConfiguration]};
      configuration.timeoutIntervalForResource =
          static_cast<double>(this->timeout_.count()) / 1000.0;
      if (this->connect_timeout_.has_value()) {
        configuration.timeoutIntervalForRequest =
            static_cast<double>(this->connect_timeout_.value().count()) /
            1000.0;
      }

      // The delegate completes before the semaphore is signalled, so
      // pointing to the stack-allocated locals from it is safe
      SourcemetaCoreHTTPDelegate *delegate{
          [[SourcemetaCoreHTTPDelegate alloc] init]};
      delegate.response = &response;
      delegate.failure = &failure;
      delegate.semaphore = dispatch_semaphore_create(0);
      delegate.hasMaximumResponseSize =
          this->maximum_response_size_.has_value() ? YES : NO;
      delegate.maximumResponseSize =
          this->maximum_response_size_.value_or(0);
      delegate.followRedirects = this->follow_redirects_ ? YES : NO;
      delegate.maximumRedirects = this->maximum_redirects_;
      delegate.redirectCount = 0;

      NSURLSession *session{
          [NSURLSession sessionWithConfiguration:configuration
                                        delegate:delegate
                                   delegateQueue:nil]};
      NSURLSessionDataTask *task{[session dataTaskWithRequest:url_request]};
      [task resume];
      dispatch_semaphore_wait(delegate.semaphore, DISPATCH_TIME_FOREVER);
      [session finishTasksAndInvalidate];
    }
  }

  if (!failure.empty()) {
    throw HTTPError{this->method_, this->url_, failure};
  }

  return response;
}

} // namespace sourcemeta::core
