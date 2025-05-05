#include "source/extensions/filters/http/request_buffer/config.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "source/extensions/filters/http/request_buffer/filter.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace RequestBuffer {

using Extensions::Common::AsyncFiles::AsyncFileManager;
using Extensions::Common::AsyncFiles::AsyncFileManagerFactory;

RequestBufferFilterFactory::RequestBufferFilterFactory()
    : FactoryBase(RequestBufferFilter::filterName()) {}

Http::FilterFactoryCb RequestBufferFilterFactory::createFilterFactoryFromProtoTyped(
    const ProtoRequestBufferFilterConfig& config,
    const std::string& stats_prefix ABSL_ATTRIBUTE_UNUSED,
    Server::Configuration::FactoryContext& context) {
  auto factory =
      AsyncFileManagerFactory::singleton(&context.serverFactoryContext().singletonManager());
  auto manager = config.has_manager_config() ? factory->getAsyncFileManager(config.manager_config())
                                             : std::shared_ptr<AsyncFileManager>();
  auto filter_config = std::make_shared<RequestBufferFilterConfig>(std::move(factory),
                                                                      std::move(manager), config);
  return [filter_config = std::move(filter_config)](Http::FilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addStreamDecoderFilter(std::make_shared<RequestBufferFilter>(filter_config));
  };
}

absl::StatusOr<Router::RouteSpecificFilterConfigConstSharedPtr>
RequestBufferFilterFactory::createRouteSpecificFilterConfigTyped(
    const ProtoRequestBufferFilterConfig& config,
    Server::Configuration::ServerFactoryContext& context, ProtobufMessage::ValidationVisitor&) {
  auto factory = AsyncFileManagerFactory::singleton(&context.singletonManager());
  auto manager = config.has_manager_config() ? factory->getAsyncFileManager(config.manager_config())
                                             : std::shared_ptr<AsyncFileManager>();
  return std::make_shared<RequestBufferFilterConfig>(std::move(factory), std::move(manager),
                                                        config);
}

REGISTER_FACTORY(RequestBufferFilterFactory,
                 Server::Configuration::NamedHttpFilterConfigFactory);

} // namespace RequestBuffer
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
