#
# Makefile for the ply display
#
#

CC = g++
INCLUDES = "/usr/include/"
DSPFILTERSDIR = ../../DSPFilters/shared/DSPFilters
DSPFILTERSINCLUDES = $(DSPFILTERSDIR)/include/
FLAGS = -Wall -g -c -I$(INCLUDES) -I$(DSPFILTERSINCLUDES) -I./
LIBS = 

OBJDIR=obj
SRCDIR=.
DSPFILTERSSRCDIR=$(DSPFILTERSDIR)/source/
TARGET_NAME=DSPCPPFilters.a
OUTPUT_DIR=./

clean:
	rm -f $(OBJDIR)/*.o
	rm -f *.bin
	rm -f *~
	rm -f $(TARGET_NAME)

OBJS =  $(OBJDIR)/filterlib.o

$(OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@echo "-------- : $<"
	$(CC) $(FLAGS) $< -o $@

DSPFILTERSOBJS += $(OBJDIR)/ChebyshevII.o
DSPFILTERSOBJS += $(OBJDIR)/Bessel.o
DSPFILTERSOBJS += $(OBJDIR)/Butterworth.o
DSPFILTERSOBJS += $(OBJDIR)/ChebyshevI.o
DSPFILTERSOBJS += $(OBJDIR)/Custom.o
DSPFILTERSOBJS += $(OBJDIR)/Filter.o
DSPFILTERSOBJS += $(OBJDIR)/PoleFilter.o
DSPFILTERSOBJS += $(OBJDIR)/RootFinder.o
DSPFILTERSOBJS += $(OBJDIR)/Biquad.o
DSPFILTERSOBJS += $(OBJDIR)/Cascade.o
DSPFILTERSOBJS += $(OBJDIR)/Design.o
DSPFILTERSOBJS += $(OBJDIR)/Elliptic.o
DSPFILTERSOBJS += $(OBJDIR)/Legendre.o
DSPFILTERSOBJS += $(OBJDIR)/Param.o
DSPFILTERSOBJS += $(OBJDIR)/RBJ.o
DSPFILTERSOBJS += $(OBJDIR)/State.o

$(DSPFILTERSOBJS): $(OBJDIR)/%.o : $(DSPFILTERSSRCDIR)/%.cpp
	@echo "-------- : $<"
	$(CC) $(FLAGS) $< -o $@

all: $(OBJS) $(DSPFILTERSOBJS) $(PORTAUDIO_UTILS_OBJS)
	ar rs $(TARGET_NAME) $(OBJS) $(DSPFILTERSOBJS)

