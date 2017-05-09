cc_library(
    name = "astar-searcher",
    srcs = [
        "astar_searcher.cpp"
    ],
    hdrs = [
        "astar_searcher.h"
    ],
    deps = [
        "//urlearning/base:base",
        "//urlearning/heuristic:heuristic",
        "//urlearning/score_cache:score_cache",
        "//urlearning/priority_queue:priority_queue",
        "@boost//:algorithm",
        "@boost//:dynamic_bitset",
        "@boost//:unordered_map"
    ]

)

cc_binary(
    name = "astar",
    srcs = [
        "astar_main.cpp",
        
    ],
    deps = [
    	":astar-searcher",
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
