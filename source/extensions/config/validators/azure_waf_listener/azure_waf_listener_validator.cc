#include "source/extensions/config/validators/azure_waf_listener/azure_waf_listener_validator.h"

#include "envoy/upstream/cluster_manager.h"

#include "source/common/common/assert.h"
#include "source/common/common/utility.h"
#include "source/common/config/utility.h"

#include "envoy/extensions/common/azure_waf/v3/waf_filter.pb.h"
#include "envoy/extensions/filters/network/http_connection_manager/v3/http_connection_manager.pb.h"
#include "envoy/extensions/common/azure_waf/v3/waf_filter.pb.validate.h"
#include "envoy/extensions/common/azure_waf/v3/waf.grpc.pb.h"

#include "source/common/config/resource_name.h"

#include "grpc++/server.h"
#include "grpcpp/grpcpp.h"


namespace Envoy {
namespace Extensions {
namespace Config {
namespace Validators {

// Validation for state of the world config.
void AzureWafListenerValidator::validate(
    const Server::Instance&, const std::vector<Envoy::Config::DecodedResourcePtr>& resources) {
  std::vector<std::string> policies;
  // Iterate through each listener resource.
  for (const auto& resource : resources) {
    const envoy::config::listener::v3::Listener& listener =
        dynamic_cast<const envoy::config::listener::v3::Listener&>(resource->resource());
    // Find HttpConnectionManager filter and iterate through the list of http filters configured to check if WAF filter is configured.
    // If WAF filter is configured, collect the policy Id for validation and continue.
    for (const auto&filter_chain: listener.filter_chains()) {
      for (const auto&filter: filter_chain.filters()) {
        auto filter_config_type = filter.config_type_case();
        if (filter_config_type != envoy::config::listener::v3::Filter::ConfigTypeCase::kTypedConfig)
        {
          continue;
        }

        // Identify HttpConnectionManager filter using type url.
        if (filter.typed_config().type_url() != Envoy::Config::getTypeUrl<envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager>())
        {
          continue;
        }

        auto filter_config_proto = filter.typed_config();
        envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager connection_manager;
        if (filter_config_proto.UnpackTo(&connection_manager)) {
          for (const auto&http_filter: connection_manager.http_filters())
          {
            auto http_filter_config_type = http_filter.config_type_case();
            if (http_filter_config_type != envoy::extensions::filters::network::http_connection_manager::v3::HttpFilter::ConfigTypeCase::kTypedConfig)
            {
              continue;
            }

            // Identify WAF filter by name and unpack into PolicyConfig struct.
            if (http_filter.typed_config().type_url() != Envoy::Config::getTypeUrl<waf::PolicyConfig>())
            {
              continue;
            }

            auto proto = http_filter.typed_config();
            waf::PolicyConfig policyConfig;
            if (proto.UnpackTo(&policyConfig)) {
              // When a WAF policy is configured only for a specific http route, listener level config will have empty policy id.
              if (policyConfig.policy_id().empty())
              {
                continue;
              }
              
              policies.push_back(policyConfig.policy_id());
              goto next;
            }
          }
        }
      }
    }
    next:
      continue;
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

void AzureWafListenerValidator::validate(
    const Server::Instance&,
    const std::vector<Envoy::Config::DecodedResourcePtr>&,
    const Protobuf::RepeatedPtrField<std::string>&) {
}

} // namespace Validators
} // namespace Config
} // namespace Extensions
} // namespace Envoy
