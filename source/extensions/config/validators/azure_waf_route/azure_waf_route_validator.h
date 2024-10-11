#pragma once

#include "envoy/config/config_validator.h"
#include "envoy/extensions/config/validators/azure_waf_route/v3/azure_waf_route.pb.h"
#include "source/extensions/common/azure_waf_grpc/client.h"

#include "envoy/extensions/common/azure_waf/v3/waf.grpc.pb.h"

#include "grpc++/server.h"
#include "grpcpp/grpcpp.h"


namespace Envoy {
namespace Extensions {
namespace Config {
namespace Validators {

using WafGrpcClientPtr = std::shared_ptr<WafGrpcClient>;

/**
 * A config validator extension to validate if WAF has the policy definitions for policies referenced in route config
 */
class AzureWafRouteValidator : public Envoy::Config::ConfigValidator {
public:
  AzureWafRouteValidator(
      const envoy::extensions::config::validators::azure_waf_route::v3::AzureWafRouteValidator&
          config)
      : config_(config){
      client = std::shared_ptr<WafGrpcClient>(new WafGrpcClient(config.waf_address(), std::chrono::milliseconds(PROTOBUF_GET_MS_OR_DEFAULT(config, request_timeout, 50))));
  }

  // ConfigValidator
  void validate(const Server::Instance& server,
                const std::vector<Envoy::Config::DecodedResourcePtr>& resources) override;

  void validate(const Server::Instance& server,
                const std::vector<Envoy::Config::DecodedResourcePtr>& added_resources,
                const Protobuf::RepeatedPtrField<std::string>& removed_resources) override;

private:
  envoy::extensions::config::validators::azure_waf_route::v3::AzureWafRouteValidator config_;
  WafGrpcClientPtr client;
};

} // namespace Validators
} // namespace Config
} // namespace Extensions
} // namespace Envoy
