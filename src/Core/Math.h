#pragma once
#if !defined(__GNUC__) && !defined(__clang__)
#include <float.h>
#endif
#include <math.h>

namespace nice_gfx { namespace detail {
// Some embedded cores provide neither <cmath> nor <float.h>. GCC-compatible
// compilers supply this check without a runtime or standard-library dependency.
inline bool finiteValue(double value) {
#if defined(__GNUC__) || defined(__clang__)
  return __builtin_isfinite(value);
#else
  return value >= -DBL_MAX && value <= DBL_MAX;
#endif
}
} }
