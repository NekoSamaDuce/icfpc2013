.PHONY: clean

CXX=g++
CXXFLAGS=-std=gnu++0x -Werror -Wall -O2 -lglog -lgflags
BINARIES=genall

all: $(BINARIES)

genall: genall.cc expr.h expr_list.h
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f *.o $(BINARIES)
