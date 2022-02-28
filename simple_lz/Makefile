LIBS=-lm -lrt

all: lz

lz: lz.cpp ../
	g++ -o $@ $< ../lodepng.cpp -O2 $(LIBS)
