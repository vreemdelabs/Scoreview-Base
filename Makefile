#
#
# Makefile for scoreview
#
#

ifeq ($(OS), Windows_NT)
OPERATINGON=__WINDOWS
INCLUDES = 
else
OPERATINGON=__LINUX
INCLUDES = "/usr/include/"
endif
RELEASE=TRUE

CC = g++
ifeq ($(OPERATINGON),__LINUX)
	CHECKSTACK=-fstack-protector-all
	RESSOURCEOBJ=
	OPENCL_INCLUDES=-I"$(AMDAPPSDKROOT)/include/"
	SDL2_FLAGS=-I"/usr/local/include/"
else
	CHECKSTACK=
	RESSOURCEOBJ=appres.o
#It does not work with nvidia cards
#OPENCL_INCLUDES=-I"$(AMDAPPSDKROOT)include/"
	OPENCL_INCLUDES=-I"$(CUDA_PATH)\include"
#OPENCL_INCLUDES=
	ifdef RELEASE
		CONSOLEFLAG=
	else
		CONSOLEFLAG=-mconsole
	endif
	SDL2_FLAGS=`sdl2-config --cflags`
endif
MESSAGESINCLUDES="./messages/"
MESHINCLUDES="./meshloader/"
COMMONINCLUDES="./common/"
PORTAUDIOUTILSINCLUDES="./pautils/"

ifdef RELEASE
FLAGS = -Wall -O2 -c -D$(OPERATINGON) $(SDL2_FLAGS) $(INCLUDES) $(OPENCL_INCLUDES) -I$(MESHINCLUDES) -I$(COMMONINCLUDES) -I$(MESSAGESINCLUDES) -I$(PORTAUDIOUTILSINCLUDES)
else
FLAGS = -Wall -g  $(CHECKSTACK) -c -D_DEBUG -D$(OPERATINGON) $(SDL2_FLAGS) $(INCLUDES) $(OPENCL_INCLUDES) -I$(MESHINCLUDES) -I$(COMMONINCLUDES) -I$(MESSAGESINCLUDES) -I$(PORTAUDIOUTILSINCLUDES)
endif
ifeq ($(OPERATINGON),__WINDOWS)
LIBEVENTINCLUDES="C:/dev/libs/libevent-2.0.22-stable/include"
FLAGS += -I$(LIBEVENTINCLUDES)
FLAGS += -I"/mingw64/include/"
endif

#SDL2_DIR="/usr/local/lib/"
#LIBDIRS = -L$(SDL2_DIR)
LIBDIRS =
ifeq ($(OPERATINGON),__WINDOWS)
#OPENCL_DIR="$(AMDAPPSDKROOT)/lib/x86_64/"
OPENCL_DIR="$(CUDA_PATH)\lib\x64"
#LIBEVENTDIR="C:/dev/libs/libevent-2.0.22-stable/.libs"
LIBDIRS += -L$(LIBEVENTDIR)
LIBDIRS += -L$(OPENCL_DIR)
endif

ifeq ($(OPERATINGON),__LINUX)
LIBS += -lGL -lGLEW
LIBS += -lSDL2 -lSDL2_ttf -lSDL2_image
LIBS += -levent_pthreads
else
LIBS += -lOpenGL32 -lglew32
LIBS += `sdl2-config --libs`
LIBS += -lSDL2_ttf -lSDL2_image 
LIBS += -lWs2_32
endif
LIBS += -lportaudio
LIBS += -lsndfile -lm -lOpenCL -lpthread -ltinyxml -lprotobuf
LIBS += -levent_core -levent
LIBS += ./meshloader/meshloader.a
LIBS += ./modules/DSPFilters/shared/DSPFilters/libDSPFilters.a
LIBS += ./messages/messages.a `pkg-config --cflags --libs protobuf`

OBJDIR=obj
SRCDIR=.
COMMONSRCDIR=./common
PORTAUDIOUTILSDIR=./pautils/
appname=scoreview
ifeq ($(OPERATINGON),__LINUX)
EXE_NAME=$(appname)
else
EXE_NAME=$(appname).exe
endif
OUTPUT_DIR=./app/

clean:
	rm -f $(OBJDIR)/*.o
	rm -f *.o
	rm -f *.bin
	rm -f *~
	rm -f $(COMMONSRCDIR)/*~
	rm -f $(OUTPUT_DIR)$(EXE_NAME)

superclean: clean
	cd ./meshloader && make clean
	cmake --build ./modules/DSPFilters/shared --target clean
	cd ./messages && make clean
	cd ./dialogs/save_openFLTK && make clean
	cd ./dialogs/practiceSDLA && make clean
	cd ./dialogs/configFLTK && make clean
	cd ./dialogs/addinstrumentFLTK && make clean

audiodata.o: audiodata.h
trackrender.o: trackrender.h audiodata.h
pictures.o: pictures.h gfxareas.h
mesh.o: mesh.h gfxareas.h
gl2Dprimitives.o: gl2Dprimitives.h mesh.h pictures.h gfxareas.h
render.o renderpianofreq.o appcard.o appmouse.o: audiodata.h shared.h gfxareas.h score.h f2n.h scorerenderer.h scoreplacement.h scoreedit.h pictures.h keypress.h card.h app.h gl2Dprimitives.h env.h
score.o scorefile.o: score.h
f2n.o: f2n.h
audiofifo.o: audiofifo.h
instrumenthand.o: instrumenthand.h f2n.h
scoreplacement.o scoreplacement_violin.o scoreplacement_piano.o scoreplacement_guitar.o scorebeams.o scorepolygons.o scorerenderer.o: gfxareas.h score.h f2n.h pictures.h keypress.h scorerenderer.h scoreplacement.h scoreedit.h
scoreedit.o scoreeditnotes.o scoreedit_piano.o scoreedit_guitar.o: f2n.h pictures.h keypress.h sscoreplacement.h scoreedit.h scoreedit_piano.h scoreedit_guitar.h gfxareas.h score.h instrumenthand.h mesh.h
thrspectrometre.o: audiodata.h shared.h spectrometre.h opencldevice.h opencldft.h dftspliter.h trackrender.h
thrbandpass.o: audiodata.h shared.h filterlib.h opencldevice.h thrbandpass.h 
thrportaudio.o: audiodata.h shared.h thrportaudio.h
thrdialogs.o: thrdialogs.h
threads.o: audiodata.h shared.h gfxareas.h score.h f2n.h scorerenderer.h card.h app.h thrspectrometre.h thrbandpass.h thrportaudio.h
messages_xml_decoder.o: messages_xml_decoder.h
messages_network.o: messages_messages.h
card.o: gfxareas.h mesh.h gl2Dprimitives.h card.h
keypress.o: keypress.h
appfiltercmds.o appoffevents.o:  audiodata.h audioapi.h shared.h gfxareas.h mesh.h gl2Dprimitives.h score.h f2n.h pictures.h keypress.h instrumenthand.h scoreplacement.h scoreplacement_violin.h scoreplacement_piano.h scoreplacement_guitar.h scoreedit.h scorerenderer.h messages.h messages_network.h card.h app.h
app.o appsoundfile.o appmessages.o: audiodata.h shared.h gfxareas.h score.h f2n.h card.h app.h keypress.h scorerenderer.h scoreplacement.h scoreedit.h env.h
sdlcalls.o: sdlcalls.h app.h audiodata.h shared.h gfxareas.h score.h f2n.h card.h app.h keypress.h scorerenderer.h scoreplacement.h scoreedit.h
hann_window.o: hann_window.h
opencldevice.o: opencldevice.h
openclfir.o opencldft.o: spectrometre.h colorscale.h hann_window.h opencldevice.h opencldft.h fircoef.h filterlib.h
openclchroma.o: spectrometre.h colorscale.h hann_window.h f2n.h opencldevice.h openclchroma.h filterlib.h
dftspliter.o: audiodata.h shared.h spectrometre.h opencldevice.h opencldft.h dftspliter.h
colorscale.o: colorscale.h 
gfxareas.o: gfxareas.h
main.o: audiodata.h shared.h gfxareas.h score.h f2n.h scorerenderer.h card.h app.h sdlcalls.h threads.h

OBJS =  $(OBJDIR)/audiodata.o
OBJS += $(OBJDIR)/trackrender.o
OBJS += $(OBJDIR)/render.o
OBJS += $(OBJDIR)/renderpianofreq.o
OBJS += $(OBJDIR)/instrumenthand.o
OBJS += $(OBJDIR)/scoreplacement.o
OBJS += $(OBJDIR)/scoreplacement_violin.o
OBJS += $(OBJDIR)/scoreplacement_piano.o
OBJS += $(OBJDIR)/scoreplacement_guitar.o
OBJS += $(OBJDIR)/scorebeams.o
OBJS += $(OBJDIR)/scorerests.o
OBJS += $(OBJDIR)/scorepolygons.o
OBJS += $(OBJDIR)/scorerenderer.o
OBJS += $(OBJDIR)/scoreedit.o
OBJS += $(OBJDIR)/scoreeditnotes.o
OBJS += $(OBJDIR)/scoreedit_piano.o
OBJS += $(OBJDIR)/scoreedit_guitar.o
OBJS += $(OBJDIR)/scorefile.o
OBJS += $(OBJDIR)/thrspectrometre.o
OBJS += $(OBJDIR)/thrbandpass.o
OBJS += $(OBJDIR)/thrportaudio.o
OBJS += $(OBJDIR)/thrdialogs.o
OBJS += $(OBJDIR)/threads.o
OBJS += $(OBJDIR)/messages_network.o
OBJS += $(OBJDIR)/card.o
OBJS += $(OBJDIR)/appsoundfile.o
OBJS += $(OBJDIR)/appcard.o
OBJS += $(OBJDIR)/appfiltercmds.o
OBJS += $(OBJDIR)/appmessages.o
OBJS += $(OBJDIR)/appmouse.o
OBJS += $(OBJDIR)/appoffevents.o
OBJS += $(OBJDIR)/app.o
OBJS += $(OBJDIR)/audiofifo.o
OBJS += $(OBJDIR)/hann_window.o
OBJS += $(OBJDIR)/opencldevice.o
OBJS += $(OBJDIR)/openclfir.o
OBJS += $(OBJDIR)/opencldft.o
OBJS += $(OBJDIR)/openclchroma.o
OBJS += $(OBJDIR)/dftspliter.o
OBJS += $(OBJDIR)/colorscale.o
OBJS += $(OBJDIR)/sdlcalls.o
OBJS += $(OBJDIR)/main.o

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

PORTAUDIO_UTILS_OBJS = $(OBJDIR)/pa_ringbuffer.o

$(PORTAUDIO_UTILS_OBJS): $(OBJDIR)/%.o : $(PORTAUDIOUTILSDIR)/%.c
	@echo "-------- : $<"
	$(CC) $(FLAGS) $< -o $@

all: $(OBJS) $(COMMONOBJS) $(PORTAUDIO_UTILS_OBJS)
ifeq ($(OPERATINGON),__WINDOWS)
	windres appres.rc appres.o
endif
	$(CC) $(OBJS) $(COMMONOBJS) $(PORTAUDIO_UTILS_OBJS) $(RESSOURCEOBJ) $(LIBDIRS) $(LIBS) -o $(OUTPUT_DIR)$(EXE_NAME) $(CONSOLEFLAG)

project:
	cd ./meshloader && make all
	cmake ./modules/DSPFilters/shared
	cmake --build ./modules/DSPFilters/shared
	cd ./messages && make all
	cd ./dialogs/save_openFLTK && make all
	cd ./dialogs/practiceSDLA && make all
	cd ./dialogs/configFLTK && make all
	cd ./dialogs/addinstrumentFLTK && make all
	make all

