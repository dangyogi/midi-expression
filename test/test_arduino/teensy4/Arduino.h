// This header file is in the public domain.

#ifndef Arduino_h
#define Arduino_h

#define F(x)  (x)

#include <cctype>
#include <algorithm>
#include <stdint.h>

// Returns the minimum of 2 input numbers.
template<class A, class B>
constexpr auto min(A&& a, B&& b) -> decltype(a < b ? std::forward<A>(a) : std::forward<B>(b)) {
  return a < b ? std::forward<A>(a) : std::forward<B>(b);
}
// Returns the maximum of 2 input numbers.
template<class A, class B>
constexpr auto max(A&& a, B&& b) -> decltype(a < b ? std::forward<A>(a) : std::forward<B>(b)) {
  return a >= b ? std::forward<A>(a) : std::forward<B>(b);
}

#include "WProgram.h"
//#include "pins_arduino.h"

#endif
