load("@envoy_api//bazel:api_build_system.bzl", "api_proto_package")

licenses(["notice"])  # Apache 2

api_proto_package(
    has_services = True,
    deps = [
        "@envoy_api//envoy/config/core/v3:pkg",
        "@envoy_api//envoy/config/common/mutation_rules/v3:pkg",
        "@envoy_api//envoy/type/matcher/v3:pkg",
        "@com_github_cncf_xds//udpa/annotations:pkg",
        "@com_github_cncf_xds//xds/annotations/v3:pkg",
    ],
)
