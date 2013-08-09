#ifndef ICFPC_UTIL_H_
#define ICFPC_UTIL_H_

#include <cassert>
#include <cstdlib>

#define NOTREACHED() {assert(false); abort();}

template<typename T>
void PrintCollection(std::ostream* os, const T& collection, const std::string& joiner) {
  if (collection.empty()) return;

  auto iter = collection.cbegin();
  *os << *iter;
  auto end = collection.cend();
  while (++iter != end) {
    *os << joiner << *iter;
  }
}

#endif  // ICFPC_UTIL_H_
