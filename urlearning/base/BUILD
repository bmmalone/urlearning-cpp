cc_library(
    name = "base",
    srcs = [
        "bayesian_network.cpp"
    ],
    hdrs = [
        "bayesian_network.h",
        "dynamic_bitset_extensions.h",
        "files.h",
        "node.h",
        "record_file.h",
        "record.h",
        "typedefs.h",
        "variable.h"
    ],
    visibility = ["//visibility:public"],
    copts = ["-Wno-sign-compare"],
    deps = [
        "@boost//:algorithm",
        "@boost//:dynamic_bitset",
        "@boost//:lexical_cast",
        "@boost//:serialization",
        "@boost//:unordered_map",

    ]
)
