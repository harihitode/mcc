TARGET=mcc
CC=gcc
CXX=g++
LLVMDIR?=/usr/local/opt/llvm
SRCS=$(wildcard *.cc)
HDRS=$(wildcard *.h)
OBJS=$(SRCS:.cc=.o)
DEPS=$(SRCS:.cc=.d)
LIBS=$(shell $(LLVMDIR)/bin/llvm-config --libs --system-libs) -lc++experimental
CXXFLAGS=-std=c++17 -MMD $(shell $(LLVMDIR)/bin/llvm-config --cflags)
LDFLAGS=$(shell $(LLVMDIR)/bin/llvm-config --ldflags)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LIBS) $(OBJS) -o $@

clean:
	$(RM) -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)
