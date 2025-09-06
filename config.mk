CC ?= gcc
AR ?= ar
CXX ?= g++

CFLAGS += -Wall
EXTRACFLAGS ?=

ifeq ($(DEBUG),1)
  CFLAGS += -ggdb3
endif

ifeq ($(ENABLE_ASAN),1)
  LDFLAGS += -fsanitize=address
  CFLAGS += -fsanitize=address
endif

