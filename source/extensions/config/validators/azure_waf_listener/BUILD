load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_extension",
    "envoy_cc_library",
    "envoy_extension_package",
)

licenses(["notice"])  # Apache 2

envoy_extension_package()

envoy_cc_library(
    name = "azure_waf_listener_validator",
    srcs = ["azure_waf_listener_validator.cc"],
    hdrs = ["azure_waf_listener_validator.h"],
    deps = [
        "//envoy/config:config_validator_interface",
        "//source/extensions/common/azure_waf_grpc:azure_waf_grpc_client_lib",
        "@envoy_api//envoy/extensions/config/validators/azure_waf_listener/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/filters/network/http_connection_manager/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/common/azure_waf/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/common/azure_waf/v3:pkg_cc_grpc",
        "//envoy/grpc:async_client_interface",
        "//envoy/grpc:async_client_manager_interface",
        "//envoy/http:filter_interface",
        "//envoy/http:header_map_interface",
        "//envoy/http:protocol_interface",
        "//envoy/network:address_interface",
        "//envoy/network:connection_interface",
        "//envoy/network:filter_interface",
        "//envoy/upstream:cluster_manager_interface",
        "//source/common/http:headers_lib",
        "//source/common/http:utility_lib",
        "//source/common/network:utility_lib",
        "//source/common/common:utility_lib",
        "//source/common/config:runtime_utility_lib",
        "//source/common/config:utility_lib",
        "//source/common/protobuf",
        "@envoy_api//envoy/config/core/v3:pkg_cc_proto",
        "@com_github_grpc_grpc//:grpc++",
        "//source/common/config:resource_name_lib",
    ],
)

envoy_cc_extension(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        ":azure_waf_listener_validator",
        "//envoy/config:config_validator_interface",
        "//envoy/registry",
        "//source/common/config:resource_name_lib",
        "@envoy_api//envoy/extensions/config/validators/azure_waf_listener/v3:pkg_cc_proto",
    ],
)
