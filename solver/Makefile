.PHONY: clean

CXX=g++
CXXFLAGS=-std=gnu++0x -Werror -Wall -O2 -lglog -lgflags
TEST_CXXFLAGS=$(CXXFLAGS) -lgtest
BINARIES=genall cluster_main simplify_main batch_evaluate
TEST_BINARIES=batch_evaluate_unittest

all: $(BINARIES) $(TEST_BINARIES)

genall: genall.cc expr.h expr_list.h
	$(CXX) $< $(CXXFLAGS) -o $@

cluster_main: cluster_main.cc expr.h expr_list.h cluster.h
	$(CXX) $< $(CXXFLAGS) -o $@

simplify_main: simplify_main.cc expr.h expr_list.h cluster.h simplify.h
	$(CXX) $< $(CXXFLAGS) -o $@

batch_evaluate: batch_evaluate.cc expr.h expr_list.h parser.h
	$(CXX) $< $(CXXFLAGS) -o $@

batch_evaluate_unittest: batch_evaluate_unittest.cc expr.h expr_list.h parser.h
	$(CXX) $< $(TEST_CXXFLAGS) -o $@

clean:
	rm -f *.o $(BINARIES)
