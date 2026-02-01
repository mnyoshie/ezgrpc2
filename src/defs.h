#ifndef EZGRPC2_DEFS_H
#define EZGRPC2_DEFS_H

#include <stdlib.h>

#define REALIGN(alignment, size) ((size) + ((alignment) - ((size) & ((alignment) - 1))))

#ifdef __ANDROID__
#include <android/api-level.h>
#if __ANDROID_API__ < 28
static inline void *aligned_alloc(size_t alignment, size_t size){
  void *ret = NULL;
  if (posix_memalign(&ret, alignment, size))
    return NULL;
  return ret;
}

#endif /* __ANDROID_API__ < 28 */
#endif /* __ANDROID__ */

#ifndef EZGRPC2_API
#define EZGRPC2_API __attribute__((visibility("default")))
#endif

#ifndef CONFIG_PATH_MAX_LEN
#define CONFIG_PATH_MAX_LEN 32
#endif

#ifndef CONFIG_IO_METHOD
#define CONFIG_IO_METHOD 0
#endif

#ifndef CONFIG_STREAM_MAX_HEADERS
#define CONFIG_STREAM_MAX_HEADERS 32
#endif

#endif

