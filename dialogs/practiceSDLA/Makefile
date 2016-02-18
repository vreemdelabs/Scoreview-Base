#
# Makefile
#
#

ifeq ($(OS), Windows_NT)
OPERATINGON=__WINDOWS
else
OPERATINGON=__LINUX
endif
RELEASE=TRUE

CC = g++
INCLUDES = "/usr/include/"

ifeq ($(OPERATINGON),__LINUX)
	SDL2_FLAGS=-I"/usr/local/include/"
else
	ifdef RELEASE
		CONSOLEFLAG=
	else
		CONSOLEFLAG=-mconsole
	endif
	SDL2_FLAGS=`sdl2-config --cflags`
endif
MESSAGESINCLUDES="../../messages/"
MESHINCLUDES="../../meshloader/"
COMMONINCLUDES="../../common/"

ifdef RELEASE
FLAGS = -Wall -O2 -c -D$(OPERATINGON) $(SDL2_FLAGS) -I$(INCLUDES) -I$(MESHINCLUDES) -I$(COMMONINCLUDES) -I$(MESSAGESINCLUDES)
else
FLAGS = -Wall -g -c -D_DEBUG -D$(OPERATINGON) $(SDL2_FLAGS) -I$(INCLUDES) -I$(MESHINCLUDES) -I$(COMMONINCLUDES) -I$(MESSAGESINCLUDES)
endif
#FLAGS += -I$(TINYXMLINCLUDES)
ifeq ($(OPERATINGON),__WINDOWS)
LIBEVENTINCLUDES="C:/dev/libs/libevent-2.0.22-stable/include"
FLAGS += -I$(LIBEVENTINCLUDES)
endif

#SDL2_DIR="/usr/local/lib/"
#LIBDIRS = -L$(SDL2_DIR)
LIBDIRS =
ifeq ($(OPERATINGON),__WINDOWS)
OPENCL_DIR="$(AMDAPPSDKROOT)/lib/x86_64/"
LIBEVENTDIR="C:/dev/libs/libevent-2.0.22-stable/.libs"
LIBDIRS += -L$(LIBEVENTDIR)
endif

ifeq ($(OPERATINGON),__LINUX)
LIBS += -lGL -lGLEW
LIBS += -lSDL2 -lSDL2_ttf -lSDL2_image
else
LIBS += -lOpenGL32 -lglew32
LIBS += `sdl2-config --libs`
LIBS += -lSDL2_ttf -lSDL2_image 
LIBS += -lWs2_32
endif
LIBS += -lpthread -ltinyxml
LIBS += -lSDL2_net
LIBS += ../../meshloader/meshloader.a
LIBS += ../../messages/messages.a `pkg-config --cflags --libs protobuf`

OBJDIR=obj
SRCDIR=.
COMMONSRCDIR=../../common
appname=practice
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

#$(OBJDIR)/main.o: main.h tcpclient.h threads.h
#$(OBJDIR)/callbacks.o: callbacks.h
#$(OBJDIR)/tcpclient.o: tcpclient.h messages_xml_decoder.h
#$(OBJDIR)/threads.o: main.h tcpclientSDLnet.h threads.h
#$(OBJDIR)/vrender.o: stringcolor.h gfxareas.h mesh.h gl2Dprimitives.h score.h pictures.h f2n.h fingerender.h vrender.h 

OBJS =  $(OBJDIR)/main.o
OBJS += $(OBJDIR)/sdlcalls.o
OBJS += $(OBJDIR)/callbacks.o
OBJS += $(OBJDIR)/threads.o
OBJS += $(OBJDIR)/fingerender.o
OBJS += $(OBJDIR)/prenderer.o
OBJS += $(OBJDIR)/sirender.o
OBJS += $(OBJDIR)/vrender.o
OBJS += $(OBJDIR)/grender.o
OBJS += $(OBJDIR)/tcpclientSDLnet.o
OBJS += $(OBJDIR)/app.o
OBJS += $(OBJDIR)/appmouse.o
OBJS += $(OBJDIR)/appmessages.o

$(OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@echo "-------- : $<"
	$(CC) $(FLAGS) $< -o $@

COMMONOBJS += $(OBJDIR)/pictures.o
COMMONOBJS += $(OBJDIR)/f2n.o
COMMONOBJS += $(OBJDIR)/keypress.o
COMMONOBJS += $(OBJDIR)/gfxareas.o
COMMONOBJS += $(OBJDIR)/score.o
COMMONOBJS += $(OBJDIR)/cfgfile.o
COMMONOBJS += $(OBJDIR)/mesh.o
COMMONOBJS += $(OBJDIR)/tcp_message_receiver.o
COMMONOBJS += $(OBJDIR)/glShaders.o
COMMONOBJS += $(OBJDIR)/gl2Dprimitives.o

$(COMMONOBJS): $(OBJDIR)/%.o : $(COMMONSRCDIR)/%.cpp
	@echo "-------- : $<"
	$(CC) $(FLAGS) $< -o $@

all: $(OBJS) $(COMMONOBJS)
	$(CC) $(OBJS) $(COMMONOBJS) $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)$(EXE_NAME) $(CONSOLEFLAG)

