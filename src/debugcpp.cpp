/**
 * @file debugcpp.cpp
 * @author Giovanni Bajo <giovannibajo@gmail.com>
 * @brief Debugging Support (C++)
 */

#include "debug.h"
#include "dlfcn_internal.h"
#include "exception_internal.h"

// OpenLara: the fixed-point engine is built -fno-exceptions/-fno-rtti and never
// throws. Upstream debugcpp installs a std::terminate handler that rethrows and
// catches (pulling the C++ EH unwinder + eh_personality, ~25 KB) plus a name
// demangler via abi::__cxa_demangle (cp-demangle.o, ~46 KB). None of that can
// run here. We drop all of it: no std::set_terminate, no __dl_demangle_func.
// Crash backtraces keep file:line; only pretty C++ names are lost (decode
// mangled names offline with c++filt). ~70 KB of RDRAM .text reclaimed.
void __debug_init_cpp(void)
{
    // Intentionally empty (see file header).
}
