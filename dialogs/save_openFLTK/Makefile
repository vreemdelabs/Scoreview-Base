#
# Makefile
#
#

ifeq ($(OS), Windows_NT)
OPERATINGON=__WINDOWS
INCLUDES = 
else
OPERATINGON=__LINUX
INCLUDES = "-I /usr/include/"
endif
RELEASE=TRUE

CC = g++


ifeq ($(OPERATINGON),__LINUX)
else
	ifdef RELEASE
		CONSOLEFLAG=
	else
		CONSOLEFLAG=-mconsole
	endif
endif
#TINYXMLINCLUDES="./tinybind-master/tinyxml/"
MESSAGESINCLUDES="../../messages/"
COMMONINCLUDES="../../common/"

ifdef RELEASE
FLAGS = -Wall -O2 -c -D$(OPERATINGON) `fltk-config --cxxflags` $(INCLUDES) -I$(COMMONINCLUDES) -I$(MESSAGESINCLUDES)
else
FLAGS = -Wall -g -c -D_DEBUG -D$(OPERATINGON) `fltk-config --cxxflags` $(INCLUDES) -I$(COMMONINCLUDES) -I$(MESSAGESINCLUDES)
endif
ifeq ($(OPERATINGON),__WINDOWS)
#FLAGS += -I"/mingw64/include/"
#LIBEVENTINCLUDES="C:/dev/libs/libevent-2.0.22-stable/include"
#FLAGS += -I$(LIBEVENTINCLUDES)
endif

#SDL2_DIR="/usr/local/lib/"
#LIBDIRS = -L$(SDL2_DIR)
LIBDIRS =
ifeq ($(OPERATINGON),__WINDOWS)
LIBEVENTDIR="C:/dev/libs/libevent-2.0.22-stable/.libs"
LIBDIRS += -L$(LIBEVENTDIR)
endif

ifeq ($(OPERATINGON),__LINUX)
LIBS += -lGL
LIBS += -lSDL2 -lSDL2_ttf -lSDL2_image
LIBS += -levent_pthreads
else
LIBS += -lOpenGL32 
LIBS += `sdl2-config --libs`
LIBS += -lSDL2_ttf -lSDL2_image 
LIBS += -lWs2_32
endif
LIBS += -lpthread -ltinyxml
LIBS += -levent_core -levent
LIBS += `fltk-config --use-images --ldflags`
LIBS += ../../messages/messages.a `pkg-config --cflags --libs protobuf`

OBJDIR=obj
SRCDIR=.
COMMONSRCDIR=../../common
appname=storedialog
ifeq ($(OPERATINGON),__LINUX)
EXE_NAME=$(appname).bin
else
EXE_NAME=$(appname).exe
endif
OUTPUT_DIR=../../app/

clean:
	rm -f $(OBJDIR)/*.o
	rm -f *.bin
	rm -f *~
	rm -f COMMONSRCDIR/*~
	rm -f $(OUTPUT_DIR)$(EXE_NAME)

main.o: main.h tcpclient.h threads.h
callbacks.o: callbacks.h
messages_xml_decoder.o: messages_xml_decoder.h
tcpclient.o: tcpclient.h
threads.o: main.h tcpclient.h threads.h
cfgfile.o: cfgfile.h

OBJS =  $(OBJDIR)/main.o
OBJS += $(OBJDIR)/callbacks.o
OBJS += $(OBJDIR)/threads.o

$(OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@echo "-------- : $<"
	$(CC) $(FLAGS) $< -o $@

COMMONOBJS += $(OBJDIR)/cfgfile.o
COMMONOBJS += $(OBJDIR)/score.o
COMMONOBJS += $(OBJDIR)/tcpclient.o

$(COMMONOBJS): $(OBJDIR)/%.o : $(COMMONSRCDIR)/%.cpp
	@echo "-------- : $<"
	$(CC) $(FLAGS) $< -o $@

all: $(OBJS) $(COMMONOBJS)
	$(CC) $(OBJS) $(COMMONOBJS) $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)$(EXE_NAME) $(CONSOLEFLAG)

