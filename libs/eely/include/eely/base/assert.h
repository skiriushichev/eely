#pragma once

#include <gsl/assert>

// Assert macros are appropriate,
// thus linting is disabled for `cppcoreguidelines-macro-usage`.

// Wrap gsl's `Expects` and `Ensures` in our own macro,
// so that they are noop in release builds.
// Otherwise they can heavily affect perfomance in critical paths.

#if defined(NDEBUG)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECTS(...)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ENSURES(...)
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECTS(...) Expects(__VA_ARGS__)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ENSURES(...) Ensures(__VA_ARGS__)
#endif