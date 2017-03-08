cc_library(
    name = "bfbnb-hash-internal",
    srcs = [
        "merged_temp_file.cpp",
        "previous_layer_stream.cpp"
    ],
    hdrs = [
        "merged_temp_file.cpp",
        "previous_layer_stream.cpp"
    ],
    deps = [
        "@boost//:archive",
        "@boost//:unordered",
        "//urlearning/base:base"
    ]

)

cc_binary(
    name = "bfbnb-hash",
    srcs = [
        "bfbnb_hash_main.cpp",
    ],
    deps = [
        ":bfbnb-hash-internal",
        "//urlearning/base:base",
        "//urlearning/fileio:fileio",
        "//urlearning/heuristic:heuristic",
        "//urlearning/priority_queue:priority_queue",
        "//urlearning/score_cache:score_cache",
        "@boost//:algorithm",
        "@boost//:archive",
        "@boost//:asio",
        "@boost//:dynamic_bitset",
        "@boost//:program_options",
        "@boost//:thread",
        "@boost//:timer",
        "@boost//:unordered_map"
    ],
    copts = ["-Wno-unused-variable", "-Wno-sign-compare"],
    visibility = ["//visibility:public"]
)
