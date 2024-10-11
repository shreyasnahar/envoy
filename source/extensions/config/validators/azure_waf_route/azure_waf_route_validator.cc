#include "source/extensions/config/validators/azure_waf_route/azure_waf_route_validator.h"

#include "envoy/upstream/cluster_manager.h"

#include "source/common/common/assert.h"
#include "source/common/common/utility.h"
#include "source/common/config/utility.h"

#include "envoy/extensions/common/azure_waf/v3/waf_filter.pb.h"
#include "envoy/extensions/filters/network/http_connection_manager/v3/http_connection_manager.pb.h"
#include "envoy/extensions/common/azure_waf/v3/waf_filter.pb.validate.h"
#include "envoy/extensions/common/azure_waf/v3/waf.grpc.pb.h"

#include "grpc++/server.h"
#include "grpcpp/grpcpp.h"


namespace Envoy {
namespace Extensions {
namespace Config {
namespace Validators {

// Validation for state of the world config.
void AzureWafRouteValidator::validate(
    const Server::Instance&, const std::vector<Envoy::Config::DecodedResourcePtr>& resources) {
  std::vector<std::string> policies;
  // Iterate through each RouteConfiguration resource.
  for (const auto& resource : resources) {
    const envoy::config::route::v3::RouteConfiguration& routeConfig =
        dynamic_cast<const envoy::config::route::v3::RouteConfiguration&>(resource->resource());
    // Iterate through each of the virtual host and then through each of the route configured under the virtual host.
    for (const auto&virtual_host: routeConfig.virtual_hosts()) {
        for (const auto&route: virtual_host.routes()) {
            // Check if the waf filter override is configured. If yes, unpack it to PerRoutePolicyConfig and
            // collect policy id for validation.
            auto waf_config = route.typed_per_filter_config().find("envoy.filters.http.azure_waf");
            if (waf_config != route.typed_per_filter_config().end()) {
                auto proto = waf_config->second;
                waf::PerRoutePolicyConfig policyConfig;
                if (proto.UnpackTo(&policyConfig)) {
                    policies.push_back(policyConfig.policy_id());
                }
            }
        }
    }
  }

  if (policies.size() == 0)
  {
    return;
  }

  std::string errorMessage;
  // Make grpc call to WAF to validate if the policies are present with WAF.
  bool present = client->CheckWafPolicies(policies, &errorMessage);

  if (!present)
  {
    throw EnvoyException(errorMessage);
  }
}

void AzureWafRouteValidator::validate(
    const Server::Instance&,
    const std::vector<Envoy::Config::DecodedResourcePtr>&,
    const Protobuf::RepeatedPtrField<std::string>&) {
}

} // namespace Validators
} // namespace Config
} // namespace Extensions
} // namespace Envoy
