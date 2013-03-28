CC=g++
#CXXFLAGS=--std=c++11

all: subshift

subshift: subshifttool.o
	$(CC) $(CXXFLAGS) subshifttool.o -o subshift

#subshifttool.o: subshifttool.cpp
#	$(CC) $(CXXFLAGS) subshifttool.cpp

clean:
	rm -rf *.o
