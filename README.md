Here are various examples for Magnum C++11 OpenGL engine, demonstrating Magnum
features, usage and capabilities. If you don't know what Magnum is, see
https://github.com/mosra/magnum. The examples are explained in the
documentation, which is also available online at
http://mosra.cz/blog/magnum-doc/example-index.html. Featured examples:

 * **Triangle** - *Hello World* of 3D graphics, displaying triangle with
   interpolated colors.
 * **Primitives** - Colored cube which can be rotated using mouse.
 * **Textured Triangle** - Loads texture and displays triangle with texture on
   it.
 * **Bullet** - Demonstrates integration of [Bullet Physics](http://www.bulletphysics.com)
   into Magnum
 * **Cube Map** - Demonstrates usage of cube map textures and different
   texture layers for simulation of open world.
 * **Motion Blur** - Moving spheres with motion blur.
 * **Framebuffer** - Demonstrates usage of multiple fragment shader outputs
   and framebuffer operations for displaying different color-corrected
   versions of the same image.
 * **Viewer** - Opens and displays COLLADA model.

Each example has its own README, explaining the features and key/mouse
controls, see `src/` subdirectories.

INSTALLATION
============

The building process is similar to Magnum itself - see [Magnum documentation](http://mosra.cz/blog/magnum-doc/)
for more comprehensive guide for building and crosscompiling.

Minimal dependencies
--------------------

*   C++ compiler with good C++11 support. Currently there are two compilers
    which are tested to support everything needed: **GCC** >= 4.6 and **Clang**
    >= 3.1. On Windows you can use **MinGW**, Visual Studio compiler still
    lacks some needed features.
*   **CMake** >= 2.8.8
*   **Magnum** - The engine itself
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
