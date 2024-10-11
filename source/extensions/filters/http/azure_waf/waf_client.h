#pragma once

#include <string>

#include "source/extensions/filters/http/common/pass_through_filter.h"

#include "envoy/extensions/common/azure_waf/v3/waf_filter.pb.h"
#include "envoy/extensions/common/azure_waf/v3/waf.pb.h"

#include "source/common/common/logger.h"

#include "envoy/stats/scope.h"
#include "envoy/stats/stats_macros.h"

#include "source/common/common/assert.h"
#include "source/common/common/logger.h"
#include "source/common/common/utility.h"
#include "source/common/http/codes.h"
#include "source/common/http/header_map_impl.h"

#include "source/common/grpc/typed_async_client.h"
#include "envoy/common/pure.h"
#include "envoy/config/core/v3/grpc_service.pb.h"
#include "envoy/grpc/status.h"
#include "envoy/stream_info/stream_info.h"
#include "envoy/grpc/async_client_manager.h"
#include "envoy/grpc/async_client.h"
#include "source/common/common/matchers.h"
#include "source/common/protobuf/protobuf.h"
#include "envoy/config/core/v3/base.pb.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace AzureWaf {

using WafHttpResponsePtr = std::unique_ptr<wafservice::WafDecision>;

class WafGrpcStream {
public:
  virtual ~WafGrpcStream() = default;
  virtual void send(wafservice::WafHttpRequest&& request, bool end_stream) PURE;
  // Idempotent close. Return true if it actually closed.
  virtual bool close() PURE;
};

using WafGrpcStreamPtr = std::unique_ptr<WafGrpcStream>;

class HttpWafFilterCallbacks {
public:
  virtual ~HttpWafFilterCallbacks() = default;
  virtual void onReceiveMessage(
      std::unique_ptr<wafservice::WafDecision>&& response) PURE;
  virtual void onGrpcError(Grpc::Status::GrpcStatus error) PURE;
  virtual void onGrpcClose() PURE;
};

class WafGrpcClient {
public:
  virtual ~WafGrpcClient() = default;
  virtual WafGrpcStreamPtr connect(HttpWafFilterCallbacks& callbacks,
                                           const envoy::config::core::v3::GrpcService& grpc_service,
                                           const StreamInfo::StreamInfo& stream_info) PURE;
};

class HttpWafFilterStreamImpl : public WafGrpcStream,
                                public Grpc::AsyncStreamCallbacks<wafservice::WafDecision>{
public:
  HttpWafFilterStreamImpl(Grpc::AsyncClient<wafservice::WafHttpRequest, wafservice::WafDecision>&& client,
                              HttpWafFilterCallbacks& callbacks,
                              const StreamInfo::StreamInfo& stream_info);
  void send(wafservice::WafHttpRequest&& request, bool end_stream) override;
  // Close the stream. This is idempotent and will return true if we
  // actually closed it.
  bool close() override;

  // AsyncStreamCallbacks
  void onReceiveMessage(WafHttpResponsePtr&&) override;

  // RawAsyncStreamCallbacks
  void onCreateInitialMetadata(Http::RequestHeaderMap& metadata) override;
  void onReceiveInitialMetadata(Http::ResponseHeaderMapPtr&& metadata) override;
  void onReceiveTrailingMetadata(Http::ResponseTrailerMapPtr&& metadata) override;
  void onRemoteClose(Grpc::Status::GrpcStatus status, const std::string& message) override;

private:
  HttpWafFilterCallbacks& callbacks_;
  Grpc::AsyncClient<wafservice::WafHttpRequest, wafservice::WafDecision> client_;
  Grpc::AsyncStream<wafservice::WafHttpRequest> stream_;
  Http::AsyncClient::ParentContext grpc_context_;
  bool stream_closed_ = false;
};

class WafGrpcClientImpl : public WafGrpcClient {
public:
  WafGrpcClientImpl(Grpc::AsyncClientManager& client_manager, Stats::Scope& scope);

  WafGrpcStreamPtr connect(HttpWafFilterCallbacks& callbacks,
                                   const envoy::config::core::v3::GrpcService& grpc_service,
                                   const StreamInfo::StreamInfo& stream_info) override;

private:
  Grpc::AsyncClientManager& client_manager_;
  Stats::Scope& scope_;
};

} // namespace AzureWaf
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy