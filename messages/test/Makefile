
.PHONY: all cpp clean

CC = g++
OBJDIR=obj
SRCDIR=.
COMMONINCLUDES = "../../common"
MESSAGEINCLUDES = "../"
FLAGS = -Wall -g -c -I$(COMMONINCLUDES) -I$(MESSAGEINCLUDES) -I./
COMMONSRCDIR= ../../common
MESSAGEDDIR = ../
LIBDIRS =
LIBS = ../messages.a

TARGET_NAME=test

clean:
	rm -f $(OBJDIR)/*.o
	rm -f *~
	rm -f scoreview.pb.cc
	rm -f scoreview.pb.cc
	rm -f $(TARGET_NAME)

OBJS += $(OBJDIR)/test.o

$(OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@echo "-------- : $<"
	$(CC) $(FLAGS) $< -o $@

COMMONOBJS = $(OBJDIR)/score.o

$(COMMONOBJS): $(OBJDIR)/%.o : $(COMMONSRCDIR)/%.cpp
	@echo "-------- : $<"
	$(CC) $(FLAGS) $< -o $@


all: $(COMMONOBJS) $(OBJS)
	@pkg-config --cflags protobuf  # fails if protobuf is not installed
	$(CC) $(OBJS) $(COMMONOBJS) $(LIBDIRS) $(LIBS) `pkg-config --cflags --libs protobuf` -o $(TARGET_NAME)

