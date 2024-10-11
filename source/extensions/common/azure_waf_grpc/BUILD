load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_extension_package",
)

licenses(["notice"])  # Apache 2

envoy_extension_package()

envoy_cc_library(
    name = "azure_waf_grpc_client_lib",
    srcs = ["client.cc"],
    hdrs = ["client.h"],
    deps = [
        "//source/common/common:assert_lib",
        "@envoy_api//envoy/extensions/common/azure_waf/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/common/azure_waf/v3:pkg_cc_grpc",
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
    ],
)
