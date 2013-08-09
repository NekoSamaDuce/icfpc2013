.PHONY: clean

CXX=g++
CXXFLAGS=-std=gnu++0x -Werror -Wall -O2 -lglog -lgflags
BINARIES=genall cluster_main

all: $(BINARIES)

genall: genall.cc expr.h expr_list.h
	$(CXX) $< $(CXXFLAGS) -o $@

cluster_main: cluster_main.cc expr.h expr_list.h cluster.h
	$(CXX) $< $(CXXFLAGS) -o $@

clean:
	rm -f *.o $(BINARIES)
