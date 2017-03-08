
cc_library(
    name = "scoring_function",
    srcs = [
        "bdeu_scoring_function.cpp",
        "bic_scoring_function.cpp",
        "fnml_scoring_function.cpp",
        "log_likelihood_calculator.cpp",
        "score_calculator.cpp",
    ],
    hdrs = [
        "bdeu_scoring_function.h",
        "bic_scoring_function.h",
        "constraints.h",
        "fnml_scoring_function.h",
        "log_likelihood_calculator.h",
        "score_calculator.h",
        "scoring_function.h"

    ],
    deps = [
        "//urlearning/base:base",
        "//urlearning/ad_tree:ad_tree",
        "//urlearning/score_cache:score_cache",
        "@boost//:asio",
        "@boost//:lexical_cast",
        "@boost//:thread",
        "@boost//:tokenizer",
        "@boost//:unordered_set",
        "@boost//:unordered_map"
    ],
    copts = ["-Wno-sign-compare", "-Wno-unused-variable"],
    visibility = ["//visibility:public"]
)

