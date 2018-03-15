CC = clang
CXX = clang++
LLVMDIR = ../llvm/build
TBLGEN = $(LLVMDIR)/bin/llvm-tblgen
CXXFLAGS = -std=c++1z `$(LLVMDIR)/bin/llvm-config --cflags` -resource-dir ~/ -fvisibility=hidden -O3
LIBS = `$(LLVMDIR)/bin/llvm-config --libs --system-libs`
OPTIONS = `$(LLVMDIR)/bin/llvm-config --cflags --ldflags`
TFLAGS = -I `$(LLVMDIR)/bin/llvm-config --includedir`
OBJS = main.o id.o idrel.o typing.o knormal.o alpha.o closure.o codegen.o

normal: $(OBJS) printer.h ast.h typing.h knormal.h alpha.h closure.h
	$(CXX) $(CXXFLAGS) $(OPTIONS) $(LIBS) $(OBJS) parser.o -o mcc
all: $(OBJS) printer.h ast.h typing.h parser.o
	$(CXX) $(CXXFLAGS) $(OPTIONS) $(LIBS) $(OBJS) parser.o -o mcc
clean:
	rm -f *.o mcc
