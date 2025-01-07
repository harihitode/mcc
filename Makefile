TARGET=mcc
CC=gcc
CXX=g++
LLVM_CONFIG?=/usr/local/opt/llvm/bin/llvm-config
LLVM_VERSION=$(shell $(LLVM_CONFIG) --version | cut -f 1 -d .)
SRCS=$(wildcard *.cc)
HDRS=$(wildcard *.h)
OBJS=$(SRCS:.cc=.o)
DEPS=$(SRCS:.cc=.d)
LIBS=$(shell $(LLVM_CONFIG) --libs --system-libs)
CXXFLAGS=-std=c++17 -MMD $(shell $(LLVM_CONFIG) --cflags) -DMCC_LLVM_VERSION=$(LLVM_VERSION)
LDFLAGS=$(shell $(LLVM_CONFIG) --ldflags)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LIBS) $(OBJS) -o $@

clean:
	$(RM) -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)
