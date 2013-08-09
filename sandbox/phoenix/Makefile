.PHONY: clean

CXX=g++
CXXFLAGS=-std=gnu++0x -Werror -Wall -O2
all: main main2

main: main.cc expr.h expr_list.h
	$(CXX) $(CXXFLAGS) $< -o $@

main2: main2.cc expr.h parser.h
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm *~ *.o main