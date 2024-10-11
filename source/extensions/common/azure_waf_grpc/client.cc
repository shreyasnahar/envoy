#include "source/extensions/common/azure_waf_grpc/client.h"

#include "envoy/config/core/v3/base.pb.h"
#include "envoy/extensions/common/azure_waf/v3/waf.pb.h"
#include "source/common/common/assert.h"
#include "source/common/http/headers.h"
#include "source/common/http/utility.h"
#include "source/common/network/utility.h"
#include "source/common/protobuf/protobuf.h"

#include "envoy/extensions/common/azure_waf/v3/waf_filter.pb.h"
#include "envoy/extensions/common/azure_waf/v3/waf_filter.pb.validate.h"
#include "envoy/extensions/common/azure_waf/v3/waf.grpc.pb.h"

#include "grpc++/server.h"
#include "grpcpp/grpcpp.h"

namespace Envoy {
namespace Extensions {
namespace Config {
namespace Validators {

bool WafGrpcClient::CheckWafPolicies(std::vector<std::string> policies, std::string* error_details)
{
  wafservice::CheckPoliciesRequest req;

  for (std::string p : policies)
  {
    req.add_policies(p);
  }

  wafservice::CheckPoliciesResponse resp;

  auto state = channel->GetState(false);
  if (state == grpc_connectivity_state::GRPC_CHANNEL_SHUTDOWN || state == grpc_connectivity_state::GRPC_CHANNEL_TRANSIENT_FAILURE) {
    channel = std::shared_ptr<grpc::Channel>(grpc::CreateChannel(address_, grpc::InsecureChannelCredentials()));
  }

  auto waf_stub = wafservice::WafService::NewStub(channel);

  grpc::ClientContext context;
  auto deadline = std::chrono::system_clock::now() + request_timeout_;
  context.set_deadline(deadline);

  // Make sync grpc call to WAF.
  grpc::Status status = waf_stub->PoliciesExist(&context, req, &resp);
  if (status.error_code() != grpc::StatusCode::OK || resp.policiespresent() != wafservice::CheckPoliciesResponse_PoliciesPresent_ALL)
  {
    *error_details = status.error_message();
    return false;
  }

  return true;
}
} // namespace Validators
} // namespace Config
} // namespace Extensions
} // namespace Envoy