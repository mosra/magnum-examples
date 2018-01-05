This example shows how to play spatialized audio with Magnum. The audio scene
includes a 3D sound source and a (default) listener, which are visualized as a
sphere and a box respectively, the listener initially facing the sound source.
The sound source can be moved around the listener using the keyboard.

![Audio](audio.png)

Key controls
-------------

-   **Right** / **Left** --- rotate source on the Y axis
-   **Up** / **Down** --- rotate source on the local X axis
-   **Page Up** / **Page Down** --- move the source away/towards the listener
-   **Escape** --- close the application

Credits
-------

This example was originally contributed by [Jonathan Hale](https://github.com/Squareys).
The sound file used is a Chimes Sound Effect from http://www.orangefreesounds.com/chimes-sound-effect/,
licensed under [Creative Commons Attribution 4.0 International License](http://creativecommons.org/licenses/by/4.0/).

Ports, live web version
-----------------------

A live web version is at http://magnum.graphics/showcase/audio/ . The
[ports branch](https://github.com/mosra/magnum-examples/tree/ports/src/audio)
contains additional patches for Emscripten support that aren't present in
`master` in order to keep the example code as simple as possible.
