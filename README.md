# Experimental lossless image compression
Compile:
```
make
```

builds the tools ``choh`` and ``dhoh``

Usage:

```
./choh infile.png -o outfile.hoh --speed=[0-9]
```

Decoding:
```
./dhoh infile.hoh -o outfile.png
```
