#pragma once

#include "envoy/extensions/filters/http/link_id_validation/v3/link_id_validation.pb.h"
#include "envoy/extensions/filters/http/link_id_validation/v3/link_id_validation.pb.validate.h"

#include "source/extensions/filters/http/common/factory_base.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace LinkIdValidation {

/**
 * Config registration for the Link ID Validation filter. @see NamedHttpFilterConfigFactory.
 */
class LinkIdValidationConfigFactory
    : public Common::FactoryBase<
          envoy::extensions::filters::http::link_id_validation::v3::LinkIdValidationConfig> {
public:
  LinkIdValidationConfigFactory() : FactoryBase("envoy.filters.http.link_id_validation") {}

private:
  Http::FilterFactoryCb createFilterFactoryFromProtoTyped(
      const envoy::extensions::filters::http::link_id_validation::v3::LinkIdValidationConfig&
          proto_config,
      const std::string& stats_prefix, Server::Configuration::FactoryContext& context) override;
};

} // namespace LinkIdValidation
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy