#pragma once

#include "envoy/http/filter.h"
#include "source/extensions/filters/http/common/pass_through_filter.h"
#include "source/common/common/logger.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace LinkIdValidation {

class LinkIdValidationFilter : public Http::PassThroughDecoderFilter,
                               public Logger::Loggable<Logger::Id::filter> {
public:
    // Http::StreamDecoderFilter
    Http::FilterHeadersStatus decodeHeaders(Http::RequestHeaderMap& headers, bool end_stream) override;

private:
    // Helper to reject requests
    void rejectRequest(Http::Code status_code, absl::string_view message);
};

} // namespace LinkIdValidation
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy