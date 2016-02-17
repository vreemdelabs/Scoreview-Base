#
#
# Makefile for the gl2Dprimitives tests
#
#

ifeq ($(OS), Windows_NT)
OPERATINGON=__WINDOWS
else
OPERATINGON=__LINUX
endif
#RELEASE=TRUE

CC = g++
INCLUDES = "/usr/include/"
ifeq ($(OPERATINGON),__LINUX)
	CHECKSTACK=-fstack-protector-all
	RESSOURCEOBJ=
	SDL2_FLAGS=-I"/usr/local/include/"
else
	CHECKSTACK=
	ifdef RELEASE
		CONSOLEFLAG=
	else
		CONSOLEFLAG=-mconsole
	endif
	SDL2_FLAGS=`sdl2-config --cflags`
endif
MESHINCLUDES="../../meshloader/"
COMMONINCLUDES="../"

ifdef RELEASE
FLAGS = -Wall -O2 -c -D$(OPERATINGON) $(SDL2_FLAGS) -I$(INCLUDES) -I$(MESHINCLUDES) -I$(COMMONINCLUDES)
else
FLAGS = -Wall -g  $(CHECKSTACK) -c -D_DEBUG -D$(OPERATINGON) $(SDL2_FLAGS) -I$(INCLUDES) -I$(MESHINCLUDES) -I$(COMMONINCLUDES)
endif

LIBDIRS =

ifeq ($(OPERATINGON),__LINUX)
LIBS += -lGL -lGLEW
LIBS += $(shell sdl2-config --libs) -lSDL2 -lSDL2_ttf -lSDL2_image
else
LIBS += -lOpenGL32 -lglew32
LIBS += `sdl2-config --libs`
LIBS += -lSDL2_ttf -lSDL2_image 
endif
LIBS += -lm -lpthread
LIBS += ../../meshloader/meshloader.a

OBJDIR=obj
SRCDIR=.
COMMONSRCDIR=../
appname=testgl2D
ifeq ($(OPERATINGON),__LINUX)
EXE_NAME=$(appname)
else
EXE_NAME=$(appname).exe
endif
OUTPUT_DIR=./

clean:
	rm -f $(OBJDIR)/*.o
	rm -f *.o
	rm -f *.bin
	rm -f *~
	rm -f $(COMMONSRCDIR)/*~
	rm -f $(OUTPUT_DIR)$(EXE_NAME)

OBJS += $(OBJDIR)/sdlcalls.o
OBJS += $(OBJDIR)/main.o

$(OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@echo "-------- : $<"
	$(CC) $(FLAGS) $< -o $@

COMMONOBJS += $(OBJDIR)/pictures.o
COMMONOBJS += $(OBJDIR)/gfxareas.o
COMMONOBJS += $(OBJDIR)/mesh.o
COMMONOBJS += $(OBJDIR)/glShaders.o
COMMONOBJS += $(OBJDIR)/gl2Dprimitives.o

$(COMMONOBJS): $(OBJDIR)/%.o : $(COMMONSRCDIR)/%.cpp
	@echo "-------- : $<"
	$(CC) $(FLAGS) $< -o $@

all: $(OBJS) $(COMMONOBJS) $(PORTAUDIO_UTILS_OBJS)
ifeq ($(OPERATINGON),__WINDOWS)
	windres appres.rc appres.o
endif
	$(CC) $(OBJS) $(COMMONOBJS) $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)$(EXE_NAME)
