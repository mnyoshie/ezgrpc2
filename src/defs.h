#ifndef EZGRPC2_DEFS_H
#define EZGRPC2_DEFS_H

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

