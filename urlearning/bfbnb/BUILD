cc_binary(
    name = "bfbnb",
    srcs = [
        "bfbnb_main.cpp"
    ],
    deps = [
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
