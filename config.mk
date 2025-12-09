CC ?= gcc
AR ?= ar
CXX ?= g++

# 0: poll
# 1: epoll
IO_METHOD ?= 0

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
  LDFLAGS += -fsanitize=address,undefined
  CFLAGS += -fsanitize=address,undefined
endif

