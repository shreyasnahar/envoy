#include "source/extensions/config/validators/azure_waf_route/config.h"

#include "envoy/extensions/config/validators/azure_waf_route/v3/azure_waf_route.pb.h"
#include "envoy/extensions/config/validators/azure_waf_route/v3/azure_waf_route.pb.validate.h"
#include "envoy/config/route/v3/route.pb.h"
#include "envoy/registry/registry.h"

#include "source/common/config/resource_name.h"

namespace Envoy {
namespace Extensions {
namespace Config {
namespace Validators {

Envoy::Config::ConfigValidatorPtr AzureWafRouteValidatorFactory::createConfigValidator(
    const ProtobufWkt::Any& config, ProtobufMessage::ValidationVisitor& validation_visitor) {
  const auto& validator_config = MessageUtil::anyConvertAndValidate<
      envoy::extensions::config::validators::azure_waf_route::v3::AzureWafRouteValidator>(
      config, validation_visitor);

  return std::make_unique<AzureWafRouteValidator>(validator_config);
}

Envoy::ProtobufTypes::MessagePtr AzureWafRouteValidatorFactory::createEmptyConfigProto() {
  return std::make_unique<
      envoy::extensions::config::validators::azure_waf_route::v3::AzureWafRouteValidator>();
}

std::string AzureWafRouteValidatorFactory::name() const {
  return absl::StrCat(category(), ".azure_waf_route_validator");
}

std::string AzureWafRouteValidatorFactory::typeUrl() const {
  // This validator is applicable to route resources.
  return Envoy::Config::getTypeUrl<envoy::config::route::v3::RouteConfiguration>();
}

/**
 * Static registration for this config validator factory. @see RegisterFactory.
 */
REGISTER_FACTORY(AzureWafRouteValidatorFactory,
                 Envoy::Config::ConfigValidatorFactory){"envoy.config.validators.azure_waf_route"};

} // namespace Validators
} // namespace Config
} // namespace Extensions
} // namespace Envoy
