LIBS=-lm -lrt

all: dhoh choh motion_vector colour_cache colour_cache_experiment split_rgb split_gRgBg split_rgb_left

dhoh: dhoh.cpp hoh_header.hpp panic.hpp lode_io.hpp lodepng.cpp entropy_coding.hpp symbolstats.hpp prefix_coding.hpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)

choh: choh.cpp hoh_header.hpp panic.hpp file_io.hpp lode_io.hpp lodepng.cpp entropy_coding.hpp encode.hpp numerics.hpp colour_transform.hpp colour_filters.hpp lz_matchFinder.hpp symbolstats.hpp prefix_coding.hpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)

motion_vector: motion_vector.cpp lode_io.hpp lodepng.cpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)

colour_cache: colour_cache.cpp lode_io.hpp lodepng.cpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)

colour_cache_experiment: colour_cache_experiment.cpp lode_io.hpp lodepng.cpp entropy_estimation.hpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)

split_rgb: split_rgb.cpp lode_io.hpp lodepng.cpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)

split_rgb_left: split_rgb_left.cpp lode_io.hpp lodepng.cpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)

split_gRgBg: split_gRgBg.cpp lode_io.hpp lodepng.cpp
	g++ -o $@ $< lodepng.cpp -O2 $(LIBS)
