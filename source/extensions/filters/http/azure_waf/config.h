#pragma once

#include <string>

#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"

#include "envoy/extensions/common/azure_waf/v3/waf_filter.pb.h"
#include "envoy/extensions/common/azure_waf/v3/waf_filter.pb.validate.h"
#include "source/extensions/filters/http/azure_waf/waf_filter.h"

#include "source/extensions/filters/http/common/factory_base.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace AzureWaf {

class WafHttpFilterConfigFactory : public Extensions::HttpFilters::Common::FactoryBase<waf::PolicyConfig, waf::PerRoutePolicyConfig> {
public:
  WafHttpFilterConfigFactory() : FactoryBase("envoy.filters.http.azure_waf") {}

private:
  // Creates a route specific config from the input proto message.
  absl::StatusOr<Router::RouteSpecificFilterConfigConstSharedPtr> createRouteSpecificFilterConfigTyped(
    const waf::PerRoutePolicyConfig& proto_config, Server::Configuration::ServerFactoryContext&, ProtobufMessage::ValidationVisitor&) override;

  // Creates a listener level config from the input proto message.
  Http::FilterFactoryCb createFilterFactoryFromProtoTyped(const waf::PolicyConfig& proto_config,
                                                     const std::string&,
                                                     Server::Configuration::FactoryContext& context) override;
  
  // Creates an empty config.
  ProtobufTypes::MessagePtr createEmptyConfigProto() override;

};

} // namespace AzureWaf
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy