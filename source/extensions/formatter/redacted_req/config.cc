#include "source/extensions/formatter/redacted_req/config.h"

#include "envoy/extensions/formatter/redacted_req/v3/redacted_req.pb.h"

#include "source/extensions/formatter/redacted_req/redacted_req.h"

namespace Envoy {
namespace Extensions {
namespace Formatter {

::Envoy::Formatter::CommandParserPtr RedactedReqFactory::createCommandParserFromProto(
    const Protobuf::Message&, Server::Configuration::GenericFactoryContext&) {
  return std::make_unique<RedactedReqCommandParser>();
}

ProtobufTypes::MessagePtr RedactedReqFactory::createEmptyConfigProto() {
  return std::make_unique<envoy::extensions::formatter::redacted_req::v3::RedactedReq>();
}

std::string RedactedReqFactory::name() const { return "envoy.formatter.redacted_req"; }

REGISTER_FACTORY(RedactedReqFactory, ::Envoy::Formatter::CommandParserFactory);

} // namespace Formatter
} // namespace Extensions
} // namespace Envoy