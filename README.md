# Seam Carver
Seam carver is an outrageously cool algorithm for fluid resizing of images. In it's current state it only calculates the gradient magnitude and luminance image of the input image. The actual seam cutting is something I'll be working on in the coming days and weeks. 

This repository contains -O3 compiled object files for quick testing purposes. It makes it really easy to recompile with -O3 enabled.

In the current state you cannot yet give it an image path and see the realtime resizing. This will be a feature for in the near future. 

## Quick start

```
make seam
./main
```

## Please note

The idea and code are both heavily inspired by (Tsoding)[https://github.com/tsoding/seam-carving]

This project has not been tested on Windows or Mac, only Ubuntu linux. As far as support goes, you may always send a pull request or add an issue to this repository.
