#pragma once

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

using WafHttpResponsePtr = std::unique_ptr<wafservice::WafDecision>;

/**
 * All stats for the waf filter. @see stats_macros.h
 */
// clang-format off
#define ALL_WAF_FILTER_STATS(GAUGE)                                                           \
  GAUGE(reference, Accumulate)
// clang-format on

/**
 * Wrapper struct for tap filter stats. @see stats_macros.h
 */
struct FilterStats {
  ALL_WAF_FILTER_STATS(GENERATE_GAUGE_STRUCT)
};

const std::string waf_stats_prefix = "envoy.waf_policy.";
const std::string waf_grpc_connection_error_msg = "WAF internal error. Unable to establish grpc connection with waf service.";

class HttpWafFilterConfig {
public:
  HttpWafFilterConfig(const waf::PolicyConfig& proto_config,
                      Stats::Scope& scope);

  const std::string& policyId() const { return policyid_; }
  const std::string& configId() const { return configid_; }
  const std::string& scopeName() const { return scopename_; }
  const std::string& scope() const { return scope_; }

private:
  const std::string policyid_;
  const std::string configid_;
  const std::string scopename_;
  const std::string scope_;
  FilterStats stats_;
};

class HttpWafPerRouteFilterConfig : public Router::RouteSpecificFilterConfig {
public:
  HttpWafPerRouteFilterConfig(const waf::PerRoutePolicyConfig& config,
                              Stats::Scope& scope);
  const std::string& policyId() const { return policyid_; }
  const std::string& configId() const { return configid_; }
  const std::string& scopeName() const { return scopename_; }
  const std::string& scope() const { return scope_; }

private:
  const std::string policyid_;
  const std::string configid_;
  const std::string scopename_;
  const std::string scope_;
  // Hold a Scope for the lifetime of the configuration
  Stats::ScopeSharedPtr stats_scope_;
  FilterStats stats_;
};


using HttpWafFilterConfigSharedPtr = std::shared_ptr<HttpWafFilterConfig>;
using WafGrpcClientPtr = std::unique_ptr<WafGrpcClientImpl>;

class HttpWafFilter : public Http::PassThroughDecoderFilter,
                                public HttpWafFilterCallbacks,
                                public Logger::Loggable<Logger::Id::http2>
{
  enum class StreamOpenState {
    // The stream was opened successfully
    Ok,
    // The stream was not opened successfully and an error was delivered
    // downstream -- processing should stop
    Error,
    // The stream was not opened successfully but processing should
    // continue as if the stream was already closed.
    IgnoreError,
  };

public:
  HttpWafFilter(HttpWafFilterConfigSharedPtr config, WafGrpcClientPtr client,
         const envoy::config::core::v3::GrpcService& grpc_service);
  ~HttpWafFilter();

  static FilterStats generateStats(const std::string policyId, Stats::Scope& scope);

  // Http::StreamFilterBase
  void onDestroy() override;

  // Http::StreamDecoderFilter
  Http::FilterHeadersStatus decodeHeaders(Http::RequestHeaderMap&, bool) override;
  Http::FilterDataStatus decodeData(Buffer::Instance&, bool) override;
  void setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks&) override;

  void onReceiveMessage(
      std::unique_ptr<wafservice::WafDecision>&& response) override;
  void onGrpcError(Grpc::Status::GrpcStatus error) override;
  void onGrpcClose() override;

private:
  const HttpWafFilterConfigSharedPtr config_;
  Http::StreamDecoderFilterCallbacks* decoder_callbacks_;
  const WafGrpcClientPtr client_;
  envoy::config::core::v3::GrpcService grpc_service_;
  WafGrpcStreamPtr stream_;
  bool responseReceived = false;
  bool endOfStreamSeen = false;
  const HttpWafPerRouteFilterConfig* per_route_config_{};

  const std::string policyId() const;
  const std::string configId() const;
  const std::string scopeName() const;
  const std::string scope() const;

  StreamOpenState openStream();
  void closeStream();
  void initPerRouteConfig();
};
} // namespace AzureWaf
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy