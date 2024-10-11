#pragma once

#include "envoy/config/config_validator.h"

#include "source/extensions/config/validators/azure_waf_route/azure_waf_route_validator.h"

namespace Envoy {
namespace Extensions {
namespace Config {
namespace Validators {

class AzureWafRouteValidatorFactory : public Envoy::Config::ConfigValidatorFactory {
public:
  AzureWafRouteValidatorFactory() = default;

  Envoy::Config::ConfigValidatorPtr
  createConfigValidator(const ProtobufWkt::Any& config,
                        ProtobufMessage::ValidationVisitor& validation_visitor) override;

  Envoy::ProtobufTypes::MessagePtr createEmptyConfigProto() override;

  std::string name() const override;

  std::string typeUrl() const override;
};

} // namespace Validators
} // namespace Config
} // namespace Extensions
} // namespace Envoy
