#include "source/extensions/config/validators/azure_waf_listener/config.h"

#include "envoy/extensions/config/validators/azure_waf_listener/v3/azure_waf_listener.pb.h"
#include "envoy/extensions/config/validators/azure_waf_listener/v3/azure_waf_listener.pb.validate.h"
#include "envoy/registry/registry.h"

#include "source/common/config/resource_name.h"

namespace Envoy {
namespace Extensions {
namespace Config {
namespace Validators {

Envoy::Config::ConfigValidatorPtr AzureWafListenerValidatorFactory::createConfigValidator(
    const ProtobufWkt::Any& config, ProtobufMessage::ValidationVisitor& validation_visitor) {
  const auto& validator_config = MessageUtil::anyConvertAndValidate<
      envoy::extensions::config::validators::azure_waf_listener::v3::AzureWafListenerValidator>(
      config, validation_visitor);

  return std::make_unique<AzureWafListenerValidator>(validator_config);
}

Envoy::ProtobufTypes::MessagePtr AzureWafListenerValidatorFactory::createEmptyConfigProto() {
  return std::make_unique<
      envoy::extensions::config::validators::azure_waf_listener::v3::AzureWafListenerValidator>();
}

std::string AzureWafListenerValidatorFactory::name() const {
  return absl::StrCat(category(), ".azure_waf_listener_validator");
}

std::string AzureWafListenerValidatorFactory::typeUrl() const {
  // This validator is applicable to listener resources.
  return Envoy::Config::getTypeUrl<envoy::config::listener::v3::Listener>();
}

/**
 * Static registration for this config validator factory. @see RegisterFactory.
 */
REGISTER_FACTORY(AzureWafListenerValidatorFactory,
                 Envoy::Config::ConfigValidatorFactory){"envoy.config.validators.azure_waf_listener"};

} // namespace Validators
} // namespace Config
} // namespace Extensions
} // namespace Envoy
