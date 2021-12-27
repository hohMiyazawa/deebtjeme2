LIBS=-lm -lrt

all: dhoh choh motion_vector colour_cache colour_cache_experiment split_rgb

dhoh: dhoh.cpp hoh_header.hpp panic.hpp lode_io.hpp lodepng.cpp entropy_coding.hpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)

choh: choh.cpp hoh_header.hpp panic.hpp lode_io.hpp lodepng.cpp entropy_coding.hpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)

motion_vector: motion_vector.cpp lode_io.hpp lodepng.cpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)

colour_cache: colour_cache.cpp lode_io.hpp lodepng.cpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)

colour_cache_experiment: colour_cache_experiment.cpp lode_io.hpp lodepng.cpp entropy_estimation.hpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)

split_rgb: split_rgb.cpp lode_io.hpp lodepng.cpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)
