load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_extension",
    "envoy_cc_library",
    "envoy_extension_package",
)

licenses(["notice"])  # Apache 2

envoy_extension_package()

envoy_cc_library(
    name = "redacted_req_lib",
    srcs = ["redacted_req.cc"],
    hdrs = ["redacted_req.h"],
    deps = [
        "//source/common/formatter:substitution_format_utility_lib",
        "//source/common/formatter:substitution_formatter_lib",
        "//source/common/protobuf:utility_lib",
        "//source/extensions/filters/common/expr:evaluator_lib",
    ]
)

envoy_cc_extension(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        "//envoy/registry",
        "//source/extensions/formatter/redacted_req:redacted_req_lib",
        "@envoy_api//envoy/extensions/formatter/redacted_req/v3:pkg_cc_proto",
    ],
)