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

![Screenshot](https://github.com/delhoume/vliv/blob/main/screenshot.jpg "Screenshot")

