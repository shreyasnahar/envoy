#pragma once

#include <functional>
#include <memory>
#include <string>

#include "envoy/extensions/filters/http/request_buffer/v3/request_buffer.pb.h"
#include "envoy/extensions/filters/http/request_buffer/v3/request_buffer.pb.validate.h"
#include "envoy/server/filter_config.h"

#include "source/extensions/common/async_files/async_file_manager_factory.h"
#include "source/extensions/filters/http/common/factory_base.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace RequestBuffer {

// Config registration for the file system buffer filter. @see NamedHttpFilterConfigFactory.
class RequestBufferFilterFactory
    : public Extensions::HttpFilters::Common::FactoryBase<
          envoy::extensions::filters::http::request_buffer::v3::RequestBufferFilterConfig> {
public:
  RequestBufferFilterFactory();

  Http::FilterFactoryCb createFilterFactoryFromProtoTyped(
      const envoy::extensions::filters::http::request_buffer::v3::RequestBufferFilterConfig&
          config,
      const std::string& stats_prefix, Server::Configuration::FactoryContext& context) override;

  absl::StatusOr<Router::RouteSpecificFilterConfigConstSharedPtr>
  createRouteSpecificFilterConfigTyped(
      const envoy::extensions::filters::http::request_buffer::v3::RequestBufferFilterConfig&
          config,
      Server::Configuration::ServerFactoryContext& context,
      ProtobufMessage::ValidationVisitor& validator) override;
};

} // namespace RequestBuffer
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
