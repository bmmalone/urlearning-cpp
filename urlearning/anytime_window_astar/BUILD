cc_binary(
    name = "anytime-window-astar",
    srcs = [
        "anytime_window_astar_main.cpp"
    ],
    deps = [
        "//urlearning/base:base",
        "//urlearning/fileio:fileio",
        "//urlearning/heuristic:heuristic",
        "//urlearning/score_cache:score_cache",
        "//urlearning/priority_queue:priority_queue",
        "@boost//:algorithm",
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
