CC ?= gcc
AR ?= ar
CXX ?= g++

# 0: poll
# 1: epoll
CONFIG_IO_METHOD ?= 0
CONFIG_PATH_MAX_LEN = 64
CONFIG_STREAM_MAX_HEADERS = 32
CONFIG_CACHE_LINE_SIZE = 64


CFLAGS += -DCONFIG_PATH_MAX_LEN=$(CONFIG_PATH_MAX_LEN)
CFLAGS += -DCONFIG_IO_METHOD=$(CONFIG_IO_METHOD)
CFLAGS += -DCONFIG_STREAM_MAX_HEADERS=$(CONFIG_STREAM_MAX_HEADERS)
CFLAGS += -DCONFIG_CACHE_LINE_SIZE=$(CONFIG_CACHE_LINE_SIZE)

CFLAGS += -Wall -std=c23 -pedantic
CFLAGS += -D_FORTIFY_SOURCE=2

# enable optimization if D is not set
ifeq ($(strip $(D)),)
  CFLAGS += -O2
endif

# used in setting up in command line
EXTRACFLAGS ?=

ifeq ($(D),1)
  CFLAGS += -ggdb3
  CFLAGS += -DEZGRPC2_DEBUG
endif

ifeq ($(T),1)
  CFLAGS += -DEZGRPC2_TRACE
endif

ifeq ($(ASAN),1)
  LDFLAGS += -fsanitize=address
  CFLAGS += -fsanitize=address
endif

