#include "source/extensions/filters/http/link_id_validation/link_id_validation_filter.h"

#include "envoy/http/codes.h"
#include "source/common/network/utility.h"
#include "source/common/http/utility.h"
#include "source/common/http/headers.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"
#include <iostream>

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace LinkIdValidation {

Http::FilterHeadersStatus LinkIdValidationFilter::decodeHeaders(Http::RequestHeaderMap& headers, bool) {
    // Get the x-forwarded-for header using the correct API
    const auto xff_header_value = headers.get(Http::Headers::get().ForwardedFor);
    if (xff_header_value.empty()) {
        rejectRequest(Http::Code::BadRequest, "Bad Request: x-forwarded-for header is missing");
        return Http::FilterHeadersStatus::StopIteration;
    }

    // Extract first IP from X-Forwarded-For header
    absl::string_view xff_value = xff_header_value[0]->value().getStringView();
    absl::string_view first_ip_str = *absl::StrSplit(xff_value, ',').begin();
    first_ip_str = absl::StripAsciiWhitespace(first_ip_str);

    // Parse the IP address
    auto ip_address_obj = Network::Utility::parseInternetAddressNoThrow(std::string(first_ip_str));
    if (!ip_address_obj || !ip_address_obj->ip()) {
        ENVOY_LOG(warn, "Client Error: Malformed IP address received: {}", first_ip_str);
        rejectRequest(Http::Code::BadRequest, "Bad Request: Malformed IP address");
        return Http::FilterHeadersStatus::StopIteration;
    }

    // Skip validation for non-IPv6 addresses
    if (ip_address_obj->ip()->version() != Network::Address::IpVersion::v6) {
        ENVOY_LOG(debug, "Skipping validation for non-IPv6 address: {}", first_ip_str);
        return Http::FilterHeadersStatus::Continue;
    }

    // Get IPv6 address bytes directly - this is the correct way to access them
    const auto* ipv6 = ip_address_obj->ip()->ipv6();
    const absl::uint128 ipv6_address = ipv6->address();
    
    // IPv6 addresses are stored in network byte order (big-endian)
    // The bytes need to be extracted correctly based on the actual memory layout
    // For fd40:1805:31:a8bf:6b24:100:a02:371, the bytes should be:
    // fd 40 18 05 31 a8 bf 6b 24 01 00 0a 02 03 71
    
    // Get the bytes using the correct byte order
    const uint8_t* byte_array = reinterpret_cast<const uint8_t*>(&ipv6_address);
    
    // On little-endian systems, the bytes are reversed in the uint128
    // So we need to access them from the end
    uint8_t ipv6_bytes_array[16];
    for (int i = 0; i < 16; i++) {
        ipv6_bytes_array[i] = byte_array[15 - i];
    }
    
    // Debug logging
    ENVOY_LOG(debug, "IPv6 address bytes: {:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
              ipv6_bytes_array[0], ipv6_bytes_array[1], ipv6_bytes_array[2], ipv6_bytes_array[3],
              ipv6_bytes_array[4], ipv6_bytes_array[5], ipv6_bytes_array[6], ipv6_bytes_array[7],
              ipv6_bytes_array[8], ipv6_bytes_array[9], ipv6_bytes_array[10], ipv6_bytes_array[11],
              ipv6_bytes_array[12], ipv6_bytes_array[13], ipv6_bytes_array[14], ipv6_bytes_array[15]);
    
    // Check if address is ULA (fd00::/8)
    if (ipv6_bytes_array[0] != 0xfd) {
        ENVOY_LOG(warn, "Forbidden: Request from non-ULA (fd00::/8) IPv6 address: {} (first byte: 0x{:02x})", 
                  ip_address_obj->ip()->addressAsString(), ipv6_bytes_array[0]);
        rejectRequest(Http::Code::Forbidden, "Forbidden: IPv6 address not permitted");
        return Http::FilterHeadersStatus::StopIteration;
    }

    // Extract Link ID from bytes 3-6 (0-based indexing: bytes 2,3,4,5)
    // This matches the Lua logic: (ipv6_bytes[6] * 16777216) + (ipv6_bytes[5] * 65536) + (ipv6_bytes[4] * 256) + ipv6_bytes[3]
    // Note: Lua uses 1-based indexing, so Lua bytes[3-6] = C++ bytes[2-5]
    const uint32_t link_id = (static_cast<uint32_t>(ipv6_bytes_array[5]) << 24) |
                             (static_cast<uint32_t>(ipv6_bytes_array[4]) << 16) |
                             (static_cast<uint32_t>(ipv6_bytes_array[3]) << 8)  |
                             (static_cast<uint32_t>(ipv6_bytes_array[2]));

    ENVOY_LOG(info, "Extracted Link ID: {}", link_id);

    // Get route metadata - this matches the Lua filter's request_handle:metadata() call
    const auto route = decoder_callbacks_->route();
    if (!route || !route->routeEntry()) {
        ENVOY_LOG(critical, "Configuration Error: Route missing");
        rejectRequest(Http::Code::InternalServerError, "Internal Server Error");
        return Http::FilterHeadersStatus::StopIteration;
    }

    // Access route metadata directly (not filter-specific metadata)
    const auto& metadata = route->metadata();
    if (metadata.filter_metadata().empty()) {
        ENVOY_LOG(critical, "Configuration Error: Route metadata missing");
        rejectRequest(Http::Code::InternalServerError, "Internal Server Error");
        return Http::FilterHeadersStatus::StopIteration;
    }
    
    // Look for the metadata directly in the route metadata (not under filter-specific metadata)
    // This matches the Lua filter's lua_metadata:get("private_frontend_link_ids") call
    const auto metadata_fields = metadata.filter_metadata().find("envoy.filters.http.lua");
    
    const ProtobufWkt::Struct* lua_metadata = nullptr;
    if (metadata_fields != metadata.filter_metadata().end()) {
        lua_metadata = &metadata_fields->second;
    }

    if (!lua_metadata || lua_metadata->fields().empty()) {
        ENVOY_LOG(critical, "Configuration Error: Lua metadata missing");
        rejectRequest(Http::Code::InternalServerError, "Internal Server Error");
        return Http::FilterHeadersStatus::StopIteration;
    }

    const auto& fields = lua_metadata->fields();
    const auto it = fields.find("private_frontend_link_ids");
    if (it == fields.end() || it->second.kind_case() != ProtobufWkt::Value::kStructValue) {
        ENVOY_LOG(critical, "Configuration Error: Allowed Link IDs missing in metadata.");
        rejectRequest(Http::Code::InternalServerError, "Internal Server Error");
        return Http::FilterHeadersStatus::StopIteration;
    }

    // Validate the link ID against allowed IDs
    const auto& valid_link_ids = it->second.struct_value().fields();
    if (valid_link_ids.find(std::to_string(link_id)) == valid_link_ids.end()) {
        ENVOY_LOG(warn, "Forbidden: Client with Link ID '{}' is not in the allowed list.", link_id);
        rejectRequest(Http::Code::Forbidden, "Forbidden: Invalid Link ID");
        return Http::FilterHeadersStatus::StopIteration;
    }

    ENVOY_LOG(info, "Link ID '{}' validated successfully", link_id);
    return Http::FilterHeadersStatus::Continue;
}

void LinkIdValidationFilter::rejectRequest(Http::Code status_code, absl::string_view message) {
    // This matches the Lua filter's reject_request function which sets connection close
    decoder_callbacks_->sendLocalReply(status_code, message, 
        [](Http::ResponseHeaderMap& headers) {
            headers.addCopy(Http::Headers::get().Connection, "close");
        }, 
        absl::nullopt, "");
}

} // namespace LinkIdValidation
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy