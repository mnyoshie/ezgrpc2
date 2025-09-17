CC ?= gcc
AR ?= ar
CXX ?= g++

CFLAGS += -Wall
EXTRACFLAGS ?=

ifeq ($(D),1)
  CFLAGS += -ggdb3
endif

ifeq ($(ASAN),1)
  LDFLAGS += -fsanitize=address
  CFLAGS += -fsanitize=address
endif

