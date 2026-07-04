#!/bin/bash -eu
cd "$SRC"
$CXX $CXXFLAGS -Iinclude -std=c++17 src/crc32.cpp src/intern_cache.cpp src/manifest.cpp src/parser.cpp src/include_resolver.cpp src/evaluator.cpp src/dependency_sorter.cpp src/manifest_writer.cpp fuzz/resolve_fuzzer.cpp $LIB_FUZZING_ENGINE -o "$OUT/resolve_fuzzer"
