#
# Makefile for the ply display
#
#

CC = g++
INCLUDES = "/usr/include/"
#"../common/"
FLAGS = -Wall -g -c -I$(INCLUDES) -I./
LIBS = 

TARGET_NAME=DSPCPPFilters.a

clean:
	rm -f *.o
	rm -f *.bin
	rm -f *~

all:
	$(CC) $(FLAGS) ChebyshevII.cpp
	$(CC) $(FLAGS) Bessel.cpp
	$(CC) $(FLAGS) Butterworth.cpp
	$(CC) $(FLAGS) ChebyshevI.cpp
	$(CC) $(FLAGS) Custom.cpp
	$(CC) $(FLAGS) Filter.cpp
	$(CC) $(FLAGS) PoleFilter.cpp
	$(CC) $(FLAGS) RootFinder.cpp
	$(CC) $(FLAGS) Biquad.cpp
	$(CC) $(FLAGS) Cascade.cpp
	$(CC) $(FLAGS) Design.cpp
	$(CC) $(FLAGS) Elliptic.cpp
	$(CC) $(FLAGS) Legendre.cpp
	$(CC) $(FLAGS) Param.cpp
	$(CC) $(FLAGS) RBJ.cpp
	$(CC) $(FLAGS) State.cpp
	$(CC) $(FLAGS) filterlib.cpp
	ar rs $(TARGET_NAME) Bessel.o Butterworth.o ChebyshevI.o Custom.o Filter.o PoleFilter.o RootFinder.o Biquad.o Cascade.o Design.o Elliptic.o Legendre.o Param.o RBJ.o State.o ChebyshevII.o filterlib.o
