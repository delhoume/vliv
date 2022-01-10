# vliv
The Very Large Image Viewer for Windows

Contains the source code for the main program (win32) and three sample plugins.
Plugins for more image types are available in vlivplugins repo (PNG, TIFF, JPEG, BMP, PPM, AVI, WEBP, QOI).

It compiles as 32bit currently.

This program uses tiling and multiple levels (called pyramids) to allow massive images (gigapixel) images
loading on modest configurations, memory needed is generally only what needs to be displayed on a screen due to
load-on-demand schemes.
It works better on pre-computed pyramidal TIFFs.
Vliv is very small and very fast when loading images.

The three sample plugins show how to use API to implement dynamic loading (or computation) of data.

The installer for released version contains all compiled plugins.

Vliv has been able to display interactively (zoom-unzoom pan) a 717 gigapixel image, that's 925696 x 775168 pixels. on a very modest PC.
The pyramidal tiled TIFF is 120 Gigabytes and has 11 pre-computed zoom levels.
The image has been created from Rembrand's "The Night Watch" using custom tools.

![vliv1](https://user-images.githubusercontent.com/1763447/148680797-38c5780c-fcdb-4616-9e2d-a7f16647a9f8.png)
![vliv2](https://user-images.githubusercontent.com/1763447/148680800-cb7c4339-ee4a-4870-8977-e841e285063e.png)
