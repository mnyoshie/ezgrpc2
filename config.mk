CC ?= gcc
AR ?= ar
CXX ?= g++

EXTRACFLAGS ?=

ifeq ($(OS),Windows_NT)
	LDFLAGS += -lWs2_32
endif

ifeq ($(HAVE_SECCOMP),1)
  LDFLAGS += -lseccomp
  CFLAGS += -DHAVE_SECCOMP
endif

ifeq ($(DEBUG),1)
  CFLAGS += -ggdb3
endif

ifeq ($(ENABLE_ASAN),1)
  LDFLAGS += -fsanitize=address
  CFLAGS += -fsanitize=address
endif
