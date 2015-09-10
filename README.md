Here are various examples for Magnum C++11/C++14 OpenGL engine, demonstrating
Magnum features, usage and capabilities. If you don't know what Magnum is, see
https://github.com/mosra/magnum.

[![Build status](https://ci.appveyor.com/api/projects/status/33qdqpdc5n0au3ou/branch/master?svg=true)](https://ci.appveyor.com/project/mosra/magnum-examples/branch/master)

INSTALLATION
============

You can either use packaging scripts, which are stored in `package/`
subdirectory, or compile and install everything manually. The building process
is similar to Magnum itself - see [Magnum documentation](http://mosra.cz/blog/magnum-doc/)
for more comprehensive guide for building, packaging and crosscompiling.

Minimal dependencies
--------------------

-   C++ compiler with good C++11 support. Compilers which are tested to have
    everything needed are **GCC** >= 4.7, **Clang** >= 3.1 and **MSVC** 2015.
    On Windows you can also use **MinGW-w64**. GCC 4.6, 4.5, 4.4 and MSVC 2013
    support involves some ugly workarounds and thus is available only in
    `compatibility` branch.
*   **CMake** >= 2.8.9
*   **Corrade**, **Magnum** -- The engine itself

Compilation, installation
-------------------------

The integration library can be built and installed using these four commands:

    mkdir -p build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr ..
    make
    make install

CONTACT
=======

Want to learn more about the library? Found a bug or want to tell me an awesome
idea? Feel free to visit my website or contact me at:

*   Website -- http://mosra.cz/blog/magnum.php
*   GitHub -- https://github.com/mosra/magnum-examples
*   IRC -- join `#magnum-engine` channel on freenode
*   Google Groups -- https://groups.google.com/forum/#!forum/magnum-engine
*   Twitter -- https://twitter.com/czmosra
*   E-mail -- mosra@centrum.cz
*   Jabber -- mosra@jabbim.cz

CREDITS
=======

See [CREDITS.md](CREDITS.md) file for details. Big thanks to everyone involved!

LICENSE
=======

Magnum is licensed under MIT/Expat license, see [COPYING](COPYING) file for
details.
