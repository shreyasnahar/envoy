load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_extension",
    "envoy_cc_library",
    "envoy_extension_package",
)

licenses(["notice"])  # Apache 2

envoy_extension_package()

envoy_cc_library(
    name = "link_id_validation_filter_lib",
    srcs = ["link_id_validation_filter.cc"],
    hdrs = ["link_id_validation_filter.h"],
    deps = [
        "//envoy/http:filter_interface",
        "//envoy/http:codes_interface",
        "//source/extensions/filters/http/common:pass_through_filter_lib",
        "//source/common/network:utility_lib",
        "//source/common/http:utility_lib",
        "//source/common/http:headers_lib",
        "//source/common/common:logger_lib",
        "//source/common/protobuf:utility_lib",
        "@com_google_absl//absl/strings",
    ],
)

envoy_cc_extension(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        ":link_id_validation_filter_lib",
        "//envoy/registry",
        "//source/extensions/filters/http/common:factory_base_lib",
        "@envoy_api//envoy/extensions/filters/http/link_id_validation/v3:pkg_cc_proto",
    ],
    alwayslink = True,  # ← ADD THIS LINE
)
