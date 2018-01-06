Here are various examples for the Magnum C++11/C++14 graphics engine,
demonstrating its features, usage and capabilities.

[![Join the chat at https://gitter.im/mosra/magnum](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/mosra/magnum?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Build Status](https://travis-ci.org/mosra/magnum-examples.svg?branch=master)](https://travis-ci.org/mosra/magnum-examples)
[![Build Status](https://ci.appveyor.com/api/projects/status/33qdqpdc5n0au3ou/branch/master?svg=true)](https://ci.appveyor.com/project/mosra/magnum-examples/branch/master)
[![UNLICENSE](https://img.shields.io/badge/license-Unlicense-blue.svg)](http://unlicense.org/)

-   Project homepage — http://magnum.graphics/
-   Documentation — http://doc.magnum.graphics/
-   GitHub project page — https://github.com/mosra/magnum-examples

SUPPORTED PLATFORMS
===================

-   **Linux** and embedded Linux
-   **Windows**, **Windows RT** <sup>(*)</sup> (Store/Phone)
-   **macOS**, **iOS** <sup>(*)</sup>
-   **Android** <sup>(*)</sup>
-   **Web** <sup>(*)</sup> ([asm.js](http://asmjs.org/) or [WebAssembly](http://webassembly.org/)),
    through [Emscripten](http://kripken.github.io/emscripten-site/)

<sup>*</sup> — the [ports](https://github.com/mosra/magnum-examples/tree/ports)
branch contains additional patches for mobile and web platform support that
aren't present in `master` in order to keep the example code as simple as
possible.

BUILDING MAGNUM EXAMPLES
========================

You can either use packaging scripts, which are stored in the
[package/](https://github.com/mosra/magnum-examples/tree/master/package)
subdirectory, or compile and install everything manually. The building process
is similar to Magnum itself — see the
[building documentation](http://doc.magnum.graphics/magnum/building-examples.html)
for more comprehensive guide for building, packaging and crosscompiling.

Minimal dependencies
--------------------

-   C++ compiler with good C++11 support. Compilers which are tested to have
    everything needed are **GCC** >= 4.7, **Clang** >= 3.1 and **MSVC** >= 2015.
    On Windows you can also use **MinGW-w64**.
-   **CMake** >= 2.8.12
-   **Corrade**, **Magnum** — The engine itself

Compilation, installation
-------------------------

The examples can be built and installed using these commands:

```sh
git clone git://github.com/mosra/magnum-examples && cd magnum-examples
mkdir -p build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make -j
make install # sudo may be required
```

Only the base Triangle example is built by default, see CMake options or the
[building documentation](http://doc.magnum.graphics/magnum/building-examples.html)
for more information about particular examples and their dependencies.

Building documentation
----------------------

The documentation is written using [Doxygen](https://doxygen.org) and is built
as part of of the main Magnum documentation. Clone the projects next to each
other and build the docs using the following:

```sh
cd ../magnum # to the directory where Magnum Doxyfile is
doxygen
```

You might need to create the `build/` directory if it doesn't exist yet.
Resulting HTML documentation will be in the `build/doc/` directory. Snapshot of
the documentation is [also available for online viewing](http://doc.magnum.graphics/).

CONTACT & SUPPORT
=================

If you want to contribute to Magnum, if you spotted a bug, need a feature or
have an awesome idea, you can get a copy of the sources from GitHub and start
right away! There is the already mentioned guide about
[how to download and build Magnum](http://doc.magnum.graphics/magnum/building.html)
and also a guide about [coding style and best practices](http://doc.magnum.graphics/magnum/coding-style.html)
which you should follow to keep the library as consistent and maintainable as
possible.

-   Project homepage — http://magnum.graphics/
-   Documentation — http://doc.magnum.graphics/
-   GitHub project page — https://github.com/mosra/magnum-examples
-   Gitter community chat — https://gitter.im/mosra/magnum
-   IRC — join the `#magnum-engine` channel on freenode
-   Google Groups mailing list — magnum-engine@googlegroups.com ([archive](https://groups.google.com/forum/#!forum/magnum-engine))
-   Author's personal Twitter — https://twitter.com/czmosra
-   Author's personal e-mail — mosra@centrum.cz

See also the Magnum Project [Contact & Support page](http://magnum.graphics/contact/)
for further information.

CREDITS
=======

See the [CREDITS.md](CREDITS.md) file for details. Big thanks to everyone
involved!

LICENSE
=======

While Magnum itself and its documentation are licensed under the MIT/Expat
license, all example code is put into public domain (or UNLICENSE) to free you
from any legal obstacles when reusing the code in your apps. See the
[COPYING](COPYING) file for details.
