.PHONY: clean

AR=ar
CXX=g++
CXXFLAGS=-std=gnu++0x -Werror -Wall -O2 -lglog -lgflags
TEST_CXXFLAGS=$(CXXFLAGS) -Ithird_party/gtest/include -lpthread
BINARIES=genall cluster_main simplify_main batch_evaluate synthesis
TEST_BINARIES=unittest simplify_unittest genall_unittest
GTEST_BINARIES=libgtest.a gtest-all.o
GTEST_DIR=third_party/gtest

all: $(BINARIES) $(TEST_BINARIES)

genall: genall.cc expr.h expr_list.h
	$(CXX) $< $(CXXFLAGS) -o $@

cluster_main: cluster_main.cc expr.h expr_list.h cluster.h
	$(CXX) $< $(CXXFLAGS) -o $@

simplify_main: simplify_main.cc expr.h expr_list.h cluster.h simplify.h
	$(CXX) $< $(CXXFLAGS) -o $@

batch_evaluate: batch_evaluate.cc expr.h expr_list.h parser.h
	$(CXX) $< $(CXXFLAGS) -o $@

synthesis: synthesis.cc expr.h simplify.h
	$(CXX) $< $(CXXFLAGS) -o $@

unittest: unittest.cc test_eval.cc libgtest.a expr.h expr_list.h parser.h
	$(CXX) $(filter %.cc, $+) $(filter %.a, $+) $(TEST_CXXFLAGS) -o $@

simplify_unittest: simplify_unittest.cc libgtest.a expr.h expr_list.h cluster.h simplify.h parser.h
	$(CXX) $(filter %.cc, $+) $(filter %.a, $+) $(TEST_CXXFLAGS) -o $@

genall_unittest: genall_unittest.cc libgtest.a expr.h expr_list.h expr_list_old.h
	$(CXX) $(filter %.cc, $+) $(filter %.a, $+) $(TEST_CXXFLAGS) -o $@

libgtest.a: gtest-all.o
	$(AR) -rv $@ $<

gtest-all.o: $(GTEST_DIR)/src/gtest-all.cc
	$(CXX) -I$(GTEST_DIR)/include -I$(GTEST_DIR) -c $< -o $@

clean:
	rm -f *.o $(BINARIES)
	rm -f *.o $(TEST_BINARIES)
	rm -f *.o $(GTEST_BINARIES)
