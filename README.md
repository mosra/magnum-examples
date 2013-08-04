Here are various examples for Magnum OpenGL 3 engine, demonstrating Magnum
features, usage and capabilities. If you don't know what Magnum is,
see https://github.com/mosra/magnum. The examples are explained in the
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

You can either use packaging scripts, which are stored in package/
subdirectory, or compile and install everything manually. The building
process is similar to Magnum itself - see Magnum documentation for more
comprehensive guide for building, packaging and crosscompiling.

Minimal dependencies
--------------------

 * C++ compiler with good C++11 support. Currently there are two compilers
   which are tested to support everything needed: **GCC** >= 4.6 and **Clang**
   >= 3.1.
 * **CMake** >= 2.6
 * **OpenGL headers**, on Linux most probably shipped with Mesa
 * **GLEW** - OpenGL extension wrangler
 * **GLUT** - OpenGL utility toolkit
 * **Magnum** - The engine itself
 * **Magnum plugins** - Plugins needed by the examples, you can get them at
   https://github.com/mosra/magnum-plugins

Compilation, installation
-------------------------

The plugins can be built and installed using these four commands:

    mkdir -p build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr .. && make
    make install

CONTACT
=======

Want to learn more about the library? Found a bug or want to tell me an
awesome idea? Feel free to visit my website or contact me at:

 * Website - http://mosra.cz/blog/magnum.php
 * GitHub - https://github.com/mosra/magnum-examples
 * Twitter - https://twitter.com/czmosra
 * E-mail - mosra@centrum.cz
 * Jabber - mosra@jabbim.cz

LICENSE
=======

Magnum is licensed under MIT/Expat license, see [COPYING](COPYING) file for
details.
