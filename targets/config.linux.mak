exe_suffix=
obj_suffix=.o
lib_suffix=.a

x=$(shell uname -m)

LINUX_CFLAGS = -DTARGET_LINUX=1 -DTARGET_PROTMODE=1
ifeq ($(x),i686)
LINUX_CFLAGS += -DTARGET_BITS=32
endif
ifeq ($(x),x86_64)
LINUX_CFLAGS += -DTARGET_BITS=64
endif

CFLAGS += $(LINUX_CFLAGS)
CPPFLAGS += $(LINUX_CFLAGS)
CXXFLAGS += $(LINUX_CFLAGS)
