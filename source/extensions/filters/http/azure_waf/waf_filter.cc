#include <string>

#include "source/extensions/filters/http/azure_waf/waf_filter.h"
#include "envoy/server/filter_config.h"

#include "source/common/common/assert.h"
#include "source/common/http/utility.h"
#include "source/common/common/logger.h"


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

HttpWafFilterConfig::HttpWafFilterConfig(
    const waf::PolicyConfig& proto_config,
    Stats::Scope& scope)
    : policyid_(proto_config.policy_id()),
      configid_(proto_config.config_id()),
      scopename_(proto_config.scope_name()),
      scope_(proto_config.scope()),
      stats_(HttpWafFilter::generateStats(waf_stats_prefix + proto_config.policy_id(), scope)) {
        // If the policy id is not initialized, skip setting the stat value.
        // Policy Id can be empty when waf policy is configured at route level but not at listener level.
        if (!proto_config.policy_id().empty()) {
          stats_.reference_.set(1);
        }
    }

HttpWafPerRouteFilterConfig::HttpWafPerRouteFilterConfig(
    const waf::PerRoutePolicyConfig& proto_config,
    Stats::Scope& scope)
    : policyid_(proto_config.policy_id()),
      configid_(proto_config.config_id()),
      scopename_(proto_config.scope_name()),
      scope_(proto_config.scope()),
      stats_scope_(scope.createScope(waf_stats_prefix)),
      stats_(HttpWafFilter::generateStats(proto_config.policy_id(), *stats_scope_)) {
        stats_.reference_.set(1);
    }

HttpWafFilter::HttpWafFilter(HttpWafFilterConfigSharedPtr config, WafGrpcClientPtr client,
         const envoy::config::core::v3::GrpcService& grpc_service)
    : config_(config), client_(std::move(client)), grpc_service_(grpc_service) {
  }

HttpWafFilter::~HttpWafFilter() {}

FilterStats HttpWafFilter::generateStats(const std::string policyId, Stats::Scope& scope) {
  std::string final_prefix =  policyId + ".";
  return {ALL_WAF_FILTER_STATS(POOL_GAUGE_PREFIX(scope, final_prefix))};
}

const std::string HttpWafFilter::policyId() const {
  if (per_route_config_) {
    return per_route_config_->policyId();
  }
  return config_->policyId();
}

const std::string HttpWafFilter::configId() const {
  if (per_route_config_) {
    return per_route_config_->configId();
  }
  return config_->configId();
}

const std::string HttpWafFilter::scopeName() const {
  if (per_route_config_) {
    return per_route_config_->scopeName();
  }
  return config_->scopeName();
}

const std::string HttpWafFilter::scope() const {
  if (per_route_config_) {
    return per_route_config_->scope();
  }
  return config_->scope();
}

Http::FilterHeadersStatus HttpWafFilter::decodeHeaders(Http::RequestHeaderMap& headers, bool end_of_stream) {
  // Get route specific config for this request. Decode headers is called only once per request, so we populate per_route_config_ only once per request.
  const auto* route_local = Http::Utility::resolveMostSpecificPerFilterConfig<HttpWafPerRouteFilterConfig>(decoder_callbacks_);
  per_route_config_ = route_local;

  // If policyid is not set bypass waf filter and continue to next filter.
  if (policyId().empty())
  {
    return Http::FilterHeadersStatus::Continue;
  }

  // Create a new stream request to waf service.
  StreamOpenState state = openStream();

  // Send 500 to client if envoy is not able to establish grpc connection with waf service.
  if (state == StreamOpenState::Error) {
    ENVOY_LOG(trace, waf_grpc_connection_error_msg);
    decoder_callbacks_->sendLocalReply(Http::Code::InternalServerError, EMPTY_STRING, nullptr, absl::nullopt, EMPTY_STRING);
    return Http::FilterHeadersStatus::Continue;
  }

  // Prepare first message to send to waf service for this request with header details and other metadata.
  wafservice::WafHttpRequest req;
  auto hFirstChunk = req.mutable_headersandfirstchunk();
  hFirstChunk->set_morebodychunks(!end_of_stream);
  hFirstChunk->set_uri(std::string(headers.getPathValue()));
  hFirstChunk->set_method(std::string((headers.getMethodValue())));
  hFirstChunk->set_protocol(Http::Utility::getProtocolString(decoder_callbacks_->streamInfo().protocol().value()));
  hFirstChunk->set_transactionid(std::string(headers.getRequestIdValue()));
  hFirstChunk->set_configid(configId());
  hFirstChunk->set_uriparsed(std::string(headers.getPathValue()));
  auto metadata = hFirstChunk->mutable_metadata();
  metadata->set_policyid(policyId());
  metadata->set_scope(scope());
  metadata->set_scopename(scopeName());
  hFirstChunk->set_remoteaddr(std::string(decoder_callbacks_->streamInfo().downstreamAddressProvider().remoteAddress()->asStringView()));

  auto* mutable_headers = hFirstChunk->mutable_headers();

  headers.iterate([mutable_headers](const Http::HeaderEntry& e) {
    auto h = mutable_headers->Add();
    std::string key = std::string(e.key().getStringView());
    h->set_value(std::string(e.value().getStringView()));

    // Map :authority header to host header
    // This is done since AzWAF reads host header but enovy passes them as :authority
    if (key == ":authority") {
        h->set_key("host");
    } else {
        h->set_key(key);
    }

    return Http::HeaderMap::Iterate::Continue;
  });

  ENVOY_LOG(trace, "sending request to waf, end of stream {}", end_of_stream);
  stream_->send(std::move(req), end_of_stream);

  // If end_of_stream is set, then hold the request until we get a decision from waf service.
  if (end_of_stream) {
    endOfStreamSeen = true;
    return Http::FilterHeadersStatus::StopIteration;
  }

  // If end_of_stream is false i.e, body is present then continue to next filter.
  return Http::FilterHeadersStatus::Continue;
}

Http::FilterDataStatus HttpWafFilter::decodeData(Buffer::Instance& buf, bool end_of_stream) {
  ENVOY_LOG(trace, "waf filter received {} request body bytes, end of stream {}", buf.length(), end_of_stream);

  // If policyid is not set bypass waf filter and continue to next filter.
  // If responseReceived is set to true, then we need not send any more body chunks to waf as waf has already made a decision.
  // This happens in cases where request body length is higher than the value that the user has configured or if the body inspection limit is less than the body enforcement limit.
  if (policyId().empty() || responseReceived)
  {
    return Http::FilterDataStatus::Continue;
  }

  if (!stream_) {
    ENVOY_LOG(trace, waf_grpc_connection_error_msg);
    decoder_callbacks_->sendLocalReply(Http::Code::InternalServerError, EMPTY_STRING, nullptr, absl::nullopt, EMPTY_STRING);
    return Http::FilterDataStatus::Continue;
  }

// Prepare a message to send to waf service with the current body chunk.
  wafservice::WafHttpRequest req;
  auto bodyChunk = req.mutable_nextbodychunk();
  bodyChunk->set_bodychunk(buf.toString());
  bodyChunk->set_morebodychunks(!end_of_stream);
  stream_->send(std::move(req), end_of_stream);

  // If end_of_stream is set, then hold the request until we get a decision from waf service.
  if (end_of_stream) {
    endOfStreamSeen = true;
    return Http::FilterDataStatus::StopIterationAndBuffer;
  } else {
    // If end_of_stream is not set, then hold the request and set high watermark until we get an ack from waf service.
    return Http::FilterDataStatus::StopIterationAndWatermark;
  }
}

void HttpWafFilter::setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) {
  decoder_callbacks_ = &callbacks;
}

HttpWafFilter::StreamOpenState HttpWafFilter::openStream() {
  if (!stream_) {
    // Start a new stream request to waf service.
    stream_ = client_->connect(*this, grpc_service_, decoder_callbacks_->streamInfo());

    if (!stream_) {
      return StreamOpenState::Error;
    }
  }

  return StreamOpenState::Ok;
}

void HttpWafFilter::closeStream() {
  if (stream_) {
    stream_->close();
    stream_.reset();
  }
}

void HttpWafFilter::onDestroy() {
  closeStream();
}

// onReceiveMessage callback gets invoked when there is a response from waf service on the grpc stream.
void HttpWafFilter::onReceiveMessage(std::unique_ptr<wafservice::WafDecision>&& response) {
  switch (response->action()) {
    case wafservice::WafDecision_Action_BLOCK:
      responseReceived = true;
      decoder_callbacks_->sendLocalReply(Http::Code::Forbidden, "Access Forbidden", nullptr, absl::nullopt, EMPTY_STRING);
      break;
    // If the message is an ack from waf service send continue decoding signal to continue processing the request.
    case wafservice::WafDecision_Action_UNINITIALIZED:
      if (!endOfStreamSeen) {
        decoder_callbacks_->continueDecoding();
      }
      break;
    default:
      responseReceived = true;
      decoder_callbacks_->continueDecoding();
  }
}

void HttpWafFilter::onGrpcError(Grpc::Status::GrpcStatus) {
  ENVOY_LOG(trace, waf_grpc_connection_error_msg);
  decoder_callbacks_->sendLocalReply(Http::Code::InternalServerError, EMPTY_STRING, nullptr, absl::nullopt, EMPTY_STRING);
}

void HttpWafFilter::onGrpcClose() {
  closeStream();
}
} // namespace AzureWaf
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy