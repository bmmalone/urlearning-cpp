cc_library(
    name = "score_cache",
    srcs = [
        "score_cache.cpp",
        "sparse_parent_bitwise.cpp",
        "sparse_parent_list.cpp",
        "sparse_parent_tree.cpp"
    ],
    hdrs = [
        "best_score_calculator.h",
        "best_score_creator.h",
        "score_cache.h",
        "sparse_parent_bitwise.h",
        "sparse_parent_list.h",
        "sparse_parent_tree.h",
        "sparse_tree_node.h"
    ],
    deps = [
        "//urlearning/base:base",
        "@boost//:algorithm",
        "@boost//:dynamic_bitset",
        "@boost//:unordered_set"
    ],
    copts = ["-Wno-unused-variable", "-Wno-sign-compare"],
    visibility = ["//visibility:public"]
)
