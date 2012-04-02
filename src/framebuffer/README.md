This example demonstrates usage of multiple fragment shader outputs,
framebuffer, renderbuffers and buffered textures for displaying different
color-corrected versions of the same image, all in one shader pass. On top
left side is original image, on right side grayscale version and on bottom
color corrected version.

![Framebuffer](https://github.com/mosra/magnum-examples/raw/master/src/framebuffer/screenshot.png)

Usage
-----

The application loads and displays TGA image passed as parameter:

    ./framebufferExample image.tga

Mouse shortcuts
---------------

**Mouse wheel** will zoom the image and **mouse drag** will move the image
around for better inspection.
