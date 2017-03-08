cc_library(
    name = "heuristic",
    srcs = [
        "combined_pattern_database.cpp",
        "dynamic_pattern_database.cpp",
        "file_pattern_database.cpp",
        "static_pattern_database.cpp",
    ],
    hdrs = [
        "combined_pattern_database.h",
        "dynamic_pattern_database.h",
        "file_pattern_database.h",
        "heuristic.h",
        "heuristic_creator.h",
        "static_pattern_database.h",
    ],
    deps = [
        "@boost//:unordered_map",
        "@boost//:unordered_set",
        "//urlearning/base:base",
        "//urlearning/score_cache:score_cache"

    ],
    copts = ["-Wno-unused-variable", "-Wno-sign-compare"],
    visibility = ["//visibility:public"]
)
