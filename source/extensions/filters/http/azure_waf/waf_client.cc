#include <string>
#include "source/extensions/filters/http/azure_waf/waf_client.h"
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

static constexpr char kExternalMethod[] = "wafservice.WafService.EvalRequest";

WafGrpcClientImpl::WafGrpcClientImpl(Grpc::AsyncClientManager& client_manager,
                                                         Stats::Scope& scope)
    : client_manager_(client_manager), scope_(scope) {}


// Gets the existing client connection to waf service or creates a new connection if there is no existing connection.
WafGrpcStreamPtr WafGrpcClientImpl::connect(HttpWafFilterCallbacks& callbacks,
                                   const envoy::config::core::v3::GrpcService& grpc_service,
                                   const StreamInfo::StreamInfo& stream_info) {
   Envoy::Grpc::GrpcServiceConfigWithHashKey config_with_hash_key =
        Envoy::Grpc::GrpcServiceConfigWithHashKey(grpc_service);
  auto client_or_error = client_manager_.getOrCreateRawAsyncClientWithHashKey(config_with_hash_key, scope_, true);
  THROW_IF_NOT_OK_REF(client_or_error.status());

  Grpc::AsyncClient<wafservice::WafHttpRequest, wafservice::WafDecision> grpcClient(client_or_error.value());
  return std::make_unique<HttpWafFilterStreamImpl>(std::move(grpcClient), callbacks,
                                                       stream_info);
}


// Creates a new stream EvalRequest to stream the request to waf service.
HttpWafFilterStreamImpl::HttpWafFilterStreamImpl(
    Grpc::AsyncClient<wafservice::WafHttpRequest, wafservice::WafDecision>&& client,
    HttpWafFilterCallbacks& callbacks, const StreamInfo::StreamInfo& stream_info)
    : callbacks_(callbacks) {
  client_ = std::move(client);
  auto descriptor = Protobuf::DescriptorPool::generated_pool()->FindMethodByName(kExternalMethod);
  grpc_context_.stream_info = &stream_info;
  Http::AsyncClient::StreamOptions options;
  options.setParentContext(grpc_context_);
  stream_ = client_.start(*descriptor, *this, options);
}

// Send the message on the stream opened with waf service.
void HttpWafFilterStreamImpl::send(wafservice::WafHttpRequest&& request,
                                       bool end_stream) {
    stream_.sendMessage(std::move(request), end_stream);
}

bool HttpWafFilterStreamImpl::close() {
  if (!stream_closed_) {
    stream_.closeStream();
    stream_closed_ = true;
    stream_.resetStream();
    return true;
  }
  return false;
}

void HttpWafFilterStreamImpl::onReceiveMessage(WafHttpResponsePtr&& response) {
  callbacks_.onReceiveMessage(std::move(response));
}

void HttpWafFilterStreamImpl::onCreateInitialMetadata(Http::RequestHeaderMap&) {}
void HttpWafFilterStreamImpl::onReceiveInitialMetadata(Http::ResponseHeaderMapPtr&&) {}
void HttpWafFilterStreamImpl::onReceiveTrailingMetadata(Http::ResponseTrailerMapPtr&&) {}

void HttpWafFilterStreamImpl::onRemoteClose(Grpc::Status::GrpcStatus status,
                                                const std::string&) {
  stream_closed_ = true;
  if (status == Grpc::Status::Ok) {
    callbacks_.onGrpcClose();
  } else {
    callbacks_.onGrpcError(status);
  }
}

} // namespace AzureWaf
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy