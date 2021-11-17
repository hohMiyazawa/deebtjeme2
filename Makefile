LIBS=-lm -lrt

all: dhoh choh

dhoh: dhoh.cpp hoh_header.hpp panic.hpp lode_io.hpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)

choh: choh.cpp hoh_header.hpp panic.hpp lode_io.hpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)
