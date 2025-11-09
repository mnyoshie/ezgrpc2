CC ?= gcc
AR ?= ar
CXX ?= g++

CFLAGS += -Wall
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

