Here are various examples for Magnum C++11 OpenGL engine, demonstrating Magnum
features, usage and capabilities. If you don't know what Magnum is, see
https://github.com/mosra/magnum. The examples are explained in the
documentation, which is also available online at
http://mosra.cz/blog/magnum-doc/example-index.html.

*   **Triangle** -- *Hello World* of 3D graphics, displaying triangle with
    interpolated colors.
*   **Primitives** -- Colored cube which can be rotated using mouse.
*   **Textured Triangle** -- Loads texture and displays triangle with texture
    on it.
*   **Viewer** -- Opens and displays COLLADA model.

*   **Bullet** -- Demonstrates integration of [Bullet Physics](http://www.bulletphysics.com)
    into Magnum
*   **Cube Map** -- Demonstrates usage of cube map textures and different
    texture layers for simulation of open world.
*   **Framebuffer** -- Demonstrates usage of multiple fragment shader outputs
    and framebuffer operations for displaying different color-corrected
    versions of the same image.
*   **Motion Blur** -- Moving spheres with motion blur.
*   **Text** -- Distance-field text rendering.

Each example has its own README, explaining the features and key/mouse
controls, see `src/` subdirectories.

COMPILATION
===========

The building process is similar to Magnum itself - see [Magnum documentation](http://mosra.cz/blog/magnum-doc/)
for more comprehensive guide for building and crosscompiling.

Minimal dependencies
--------------------

-   C++ compiler with good C++11 support. Currently there are two compilers
    which are tested to have everything needed: **GCC** >= 4.6 and **Clang**
    >= 3.1. On Windows you can use **MinGW**. Most of the examples should also
    work on GCC 4.5, 4.4 and **MSVC** 2013.
*   **CMake** >= 2.8.8
*   **Magnum** - The engine itself

For some examples you might need also these (see below for more information):

*   **Magnum Plugins** - Plugins needed by some of the examples, you can get
    them at https://github.com/mosra/magnum-plugins.
*   **Magnum Integration** - Integration library needed by some of the
    examples, you can get it at https://github.com/mosra/magnum-integration.

Compilation, installation
-------------------------

The examples can be built using these three commands:

    mkdir -p build && cd build
    cmake ..
    make

Note that by default only *Triangle* example is built. Some examples depend on
plugins and integration libraries from `magnum-plugins` and `magnum-integration`
repositories. You have to enable the other examples with these CMake options
(e.g. pass `-DWITH_VIEWER=ON` to CMake):

*   `WITH_BULLET` -- Build *Bullet* example. Requires `BulletIntegration`
    library.
*   `WITH_CUBEMAP` -- Build *Cube Map* example. Requires `JpegImporter` plugin,
    not available in OpenGL ES.
*   `WITH_FRAMEBUFFER` -- Build *Framebuffer* example. Requires `TgaImporter`
    plugin, not available in OpenGL ES.
*   `WITH_MOTIONBLUR` -- Build *Motion Blur* example. Not available on OpenGL
    ES.
*   `WITH_PRIMITIVES` -- Build *Primitives* example.
*   `WITH_TEXT` -- Build *Text* example. Requires `FreeTypeFont` plugin.
*   `WITH_TEXTUREDTRIANGLE` -- Build *Textured Triangle* example. Requires
    `JpegImporter` plugin, not available in OpenGL ES.
*   `WITH_TRIANGLE` -- Build *Triangle* example.
*   `WITH_VIEWER` -- Build *Viewer* example. Requires `ColladaImporter` plugin.

CONTACT
=======

Want to learn more about the library? Found a bug or want to tell me an awesome
idea? Feel free to visit my website or contact me at:

*   Website - http://mosra.cz/blog/magnum.php
*   GitHub - https://github.com/mosra/magnum-examples
*   Twitter - https://twitter.com/czmosra
*   E-mail - mosra@centrum.cz
*   Jabber - mosra@jabbim.cz

LICENSE
=======

Magnum is licensed under MIT/Expat license, see [COPYING](COPYING) file for
details.
