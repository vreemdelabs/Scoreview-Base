
.PHONY: all cpp clean

CC = g++
OBJDIR=obj
INCLUDES = "../common"
FLAGS = -Wall -g -c -I$(INCLUDES) -I./
LIBS = 

TARGET_NAME=messages.a

clean:
	rm -f $(OBJDIR)/*.o
	rm -f *.o
	rm -f *~
	rm -f scoreview.pb.cc
	rm -f scoreview.pb.h
	rm -f $(TARGET_NAME)

test:
	pkg-config --cflags protobuf  # fails if protobuf is not installed
	cd ./test && make all

protocol:
	protoc --cpp_out=./ ./scoreview.proto

all: test protocol
	$(CC) $(FLAGS) scoreview.pb.cc
	$(CC) $(FLAGS) message_coder.cpp
	$(CC) $(FLAGS) message_decoder.cpp
	ar rs $(TARGET_NAME) message_coder.o message_decoder.o scoreview.pb.o

