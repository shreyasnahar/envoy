#include "source/extensions/filters/http/link_id_validation/config.h"

#include "source/extensions/filters/http/link_id_validation/link_id_validation_filter.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace LinkIdValidation {

Http::FilterFactoryCb LinkIdValidationConfigFactory::createFilterFactoryFromProtoTyped(
    const envoy::extensions::filters::http::link_id_validation::v3::LinkIdValidationConfig&,
    const std::string&, Server::Configuration::FactoryContext&) {
  return [](Http::FilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addStreamDecoderFilter(std::make_shared<LinkIdValidationFilter>());
  };
}

REGISTER_FACTORY(LinkIdValidationConfigFactory, Server::Configuration::NamedHttpFilterConfigFactory);

} // namespace LinkIdValidation
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy