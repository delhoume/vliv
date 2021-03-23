# vliv
The Very Large Image Viewer for Windows

Contains the source code for the main program (win32) and two sample plugins.
Plugins for more image types are available on request for a small fee (PNG, TIFF, JPEG, BMP, PPM, AVI, JPEG2000).

This program uses tiling and multiple levels (called pyramids) to allow massive images (gigapixel) images
loading on modest configurations, memory needed is generally only what needs to be displayed on a screen due to
load-on-demand schemes.

The two sample plugins show how to use API to implement dynamic loading (computation) of data
