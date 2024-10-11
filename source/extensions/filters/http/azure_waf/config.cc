#include <string>
#include <sys/stat.h>

#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"
#include "source/extensions/filters/http/azure_waf/config.h"
#include "envoy/extensions/common/azure_waf/v3/waf_filter.pb.h"
#include "envoy/extensions/common/azure_waf/v3/waf_filter.pb.validate.h"
#include "source/extensions/filters/http/azure_waf/waf_filter.h"

#include "source/extensions/filters/http/common/factory_base.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace AzureWaf {


absl::StatusOr<Router::RouteSpecificFilterConfigConstSharedPtr> WafHttpFilterConfigFactory::createRouteSpecificFilterConfigTyped(
    const waf::PerRoutePolicyConfig& proto_config, Server::Configuration::ServerFactoryContext& context, ProtobufMessage::ValidationVisitor&) {
  return std::make_shared<HttpWafPerRouteFilterConfig>(proto_config, context.scope());
}

Http::FilterFactoryCb WafHttpFilterConfigFactory::createFilterFactoryFromProtoTyped(const waf::PolicyConfig& proto_config,
                                                    const std::string&,
                                                    Server::Configuration::FactoryContext& context) {
  HttpWafFilterConfigSharedPtr config =
      std::make_shared<HttpWafFilterConfig>(
          HttpWafFilterConfig(proto_config, context.scope()));

  return [grpc_service = proto_config.grpc_service(), &context, config](Http::FilterChainFactoryCallbacks& callbacks) -> void {
    // The callback is created in main thread and executed in worker thread, variables except factory
    // context must be captured by value into the callback.
    Http::FilterFactoryCb callback;

    // Envoy gRPC client.
    auto client = std::make_unique<WafGrpcClientImpl>(
    context.serverFactoryContext().clusterManager().grpcAsyncClientManager(), context.scope());

    callbacks.addStreamDecoderFilter(Http::StreamDecoderFilterSharedPtr{std::make_shared<HttpWafFilter>(config, std::move(client), grpc_service)});
  };
}

ProtobufTypes::MessagePtr WafHttpFilterConfigFactory::createEmptyConfigProto() {
  return ProtobufTypes::MessagePtr{new waf::PolicyConfig()};
}

/**
 * Static registration for this Waf filter. @see RegisterFactory.
 */

REGISTER_FACTORY(WafHttpFilterConfigFactory, Server::Configuration::NamedHttpFilterConfigFactory);
} // namespace AzureWaf
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy