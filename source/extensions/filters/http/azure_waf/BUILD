load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_extension",
    "envoy_cc_library",
    "envoy_cc_test",
    "envoy_extension_package",
)
load("@envoy_api//bazel:api_build_system.bzl", "api_proto_package")

envoy_extension_package()


envoy_cc_library(
    name = "waf_filter_lib",
    srcs = ["waf_filter.cc", "waf_client.cc"],
    hdrs = ["waf_filter.h", "waf_client.h"],
    deps = [
        "@envoy_api//envoy/extensions/common/azure_waf/v3:pkg_cc_proto",
        "//source/extensions/filters/http/common:pass_through_filter_lib",
        "//source/common/common:assert_lib",
        "//source/common/common:logger_lib",
        "//envoy/buffer:buffer_interface",
        "//source/common/http:codes_lib",
        "//source/common/http:utility_lib",
        "//envoy/http:codes_interface",
        "//envoy/stats:stats_macros",
        "//envoy/grpc:async_client_interface",
        "//envoy/stats:stats_interface",
        "//envoy/upstream:cluster_manager_interface",
        "//source/common/grpc:typed_async_client_lib",
        "//envoy/grpc:status",
        "//envoy/stream_info:stream_info_interface",
    ],
)

envoy_cc_extension(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        ":waf_filter_lib",
        "//source/extensions/filters/http/common:factory_base_lib",
        "//envoy/server:filter_config_interface",
    ],
)