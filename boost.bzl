###
# This file is mildly adapted from the version in this repo:
#   https://github.com/nelhage/rules_boost
###

include_pattern = "boost/%s/"
hdrs_patterns = [
  "boost/%s.h",
  "boost/%s.hpp",
  "boost/%s/**/*.hpp",
  "boost/%s/**/*.ipp",
  "boost/%s/**/*.h",
  "libs/%s/src/*.ipp",
]
srcs_patterns = [
  "libs/%s/src/*.cpp",
  "libs/%s/src/*.hpp",
]

# Building boost results in many warnings for unused values. Downstream users
# won't be interested, so just disable the warning.
default_copts = ["-Wno-unused-value"]

def srcs_list(library_name):
  return native.glob([p % (library_name,) for p in srcs_patterns])

def includes_list(library_name):
  return [".", include_pattern % library_name]

def hdr_list(library_name):
  return native.glob([p % (library_name,) for p in hdrs_patterns])

def boost_library(name, defines=None, includes=None, hdrs=None, srcs=None, deps=None, copts=None, linkopts=None):
  if defines == None:
    defines = []

  if includes == None:
    includes = []

  if hdrs == None:
    hdrs = []

  if srcs == None:
    srcs = []

  if deps == None:
    deps = []

  if copts == None:
    copts = []

  if linkopts == None:
    linkopts = []

  return native.cc_library(
    name = name,
    visibility = ["//visibility:public"],
    defines = defines,
    includes = includes_list(name) + includes,
    hdrs = hdr_list(name) + hdrs,
    srcs = srcs_list(name) + srcs,
    deps = deps,
    copts = default_copts + copts,
    licenses = ["notice"],
    linkopts = linkopts,
  )

def boost_deps():
  native.new_http_archive(
    name = "boost",
    #url = "https://cytranet.dl.sourceforge.net/project/boost/boost/1.63.0/boost_1_63_0.tar.bz2",
    url = "https://downloads.sourceforge.net/project/boost/boost/1.63.0/boost_1_63_0.tar.bz2",
    build_file = "BUILD.boost",
    type = "tar.bz2",
    strip_prefix = "boost_1_63_0/",
    sha256 = "beae2529f759f6b3bf3f4969a19c2e9d6f0c503edcb2de4a61d1428519fcb3b0",
  )
