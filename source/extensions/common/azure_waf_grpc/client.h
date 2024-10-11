#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "envoy/config/core/v3/base.pb.h"
#include "envoy/network/address.h"
#include "envoy/network/connection.h"
#include "envoy/network/filter.h"

#include "envoy/extensions/common/azure_waf/v3/waf.pb.h"

#include "source/common/common/logger.h"

#include "envoy/stats/scope.h"
#include "envoy/stats/stats_macros.h"

#include "source/common/common/assert.h"
#include "source/common/common/logger.h"
#include "source/common/common/utility.h"
#include "envoy/common/pure.h"
#include "envoy/config/core/v3/grpc_service.pb.h"
#include "envoy/grpc/status.h"
#include "source/common/protobuf/protobuf.h"
#include "envoy/config/core/v3/base.pb.h"
#include "envoy/http/filter.h"
#include "envoy/http/protocol.h"

#include "envoy/extensions/common/azure_waf/v3/waf.grpc.pb.h"

#include "grpc++/server.h"
#include "grpcpp/grpcpp.h"

namespace Envoy {
namespace Extensions {
namespace Config {
namespace Validators {

using GrpcChannelPtr = std::shared_ptr<grpc::Channel>;

class WafGrpcClient {
public:
  WafGrpcClient(std::string address, std::chrono::milliseconds request_timeout)
      : address_(address), request_timeout_(request_timeout){
      channel = std::shared_ptr<grpc::Channel>(grpc::CreateChannel(address_, grpc::InsecureChannelCredentials()));
  }

  bool CheckWafPolicies(std::vector<std::string> policies, std::string* error_details);

private:
  std::string address_;
  std::chrono::milliseconds request_timeout_;
  GrpcChannelPtr channel;
};
} // namespace Validators
} // namespace Config
} // namespace Extensions
} // namespace Envoy