#include "eely/base/base_utils.h"

#if EELY_PLATFORM_WIN64
#include <windows.h>
#else
#include <cstdlib>
#endif

namespace eely::internal {
void* aligned_alloc(const size_t alignment, const size_t aligned_size)
{
#if EELY_PLATFORM_WIN64
  return _aligned_malloc(aligned_size, alignment);
#else
  return std::aligned_alloc(alignment, aligned_size);
#endif
}

void aligned_free(void* const ptr)
{
#if EELY_PLATFORM_WIN64
  return _aligned_free(ptr);
#else
  return std::free(ptr);
#endif
}
}  // namespace eely::internal