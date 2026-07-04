CXX ?= g++
CXXFLAGS ?= -Wall -Werror -Wextra -pedantic -std=c++17 -O2 -g
CPPFLAGS ?= -Iinclude

LIB_SRCS := src/crc32.cpp src/intern_cache.cpp src/manifest.cpp src/parser.cpp src/include_resolver.cpp src/evaluator.cpp src/dependency_sorter.cpp src/manifest_writer.cpp src/scenario_corpus.cpp
LIB_OBJS := $(LIB_SRCS:.cpp=.o)

.PHONY: all test clean seeds pipeline

all: cairn tools/generate_seeds tests/cairn_tests

cairn: $(LIB_OBJS) src/cli.o
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

tests/cairn_tests: $(LIB_OBJS) tests/cairn_tests.o
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

tools/generate_seeds: $(LIB_OBJS) tools/generate_seeds.o
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

test: tests/cairn_tests
	./tests/cairn_tests

seeds: tools/generate_seeds
	./tools/generate_seeds fuzz/corpus/resolve_fuzzer

pipeline: all test seeds

clean:
	rm -f $(LIB_OBJS) src/cli.o tests/cairn_tests.o tools/generate_seeds.o cairn tests/cairn_tests tools/generate_seeds
