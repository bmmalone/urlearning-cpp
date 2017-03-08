cc_binary(
    name = "score",
    srcs = [
        "score_main.cpp",
    ],
    deps = [
        "//urlearning/base:base",
        "//urlearning/ad_tree:ad_tree",
        "//urlearning/score_cache:score_cache",
        "//urlearning/scoring_function:scoring_function",
        "@boost//:program_options",
        "@boost//:thread",
        "@boost//:timer",
        "@boost//:unordered_set"
    ],
    copts = ["-Wno-unused-variable", "-Wno-sign-compare"]
)
