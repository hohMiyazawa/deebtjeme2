LIBS=-lm -lrt

all: lz lz2

lz: lz.cpp ../
	g++ -o $@ $< ../lodepng.cpp -O2 $(LIBS)

lz2: lz2.cpp ../
	g++ -o $@ $< ../lodepng.cpp -O2 $(LIBS)
