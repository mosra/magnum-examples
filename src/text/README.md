Showcase of Magnum distance-field text rendering. Instead of rendering the
glyphs for each size, the glyphs are prerendered and converted into a signed
distance field texture. The texture is then used for rendering the text. Both
glyph pre-rendering and the actual text layouting supports UTF-8. For mutable
text buffer mapping is used.

![Text](text.png)

Usage
-----

**Mouse wheel** rotates and scales the text.

Font license
------------

This example uses `DejaVuSans.ttf` font from [DejaVu Project](dejavu-fonts.org).

The font is generated using the following command:

    magnum-fontconverter --font FreeTypeFont --converter MagnumFontConverter --characters 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-+,.!°ěäЗдравстуймиΓειασουκόμ ' --font-size 110 --output-size "512 512" --radius 22 DejaVuSans.ttf DejaVuSans

Ports, live web version
-----------------------

A live web version is at http://magnum.graphics/showcase/text/ . The
[ports branch](https://github.com/mosra/magnum-examples/tree/ports/src/text)
contains additional patches for Emscripten support that aren't present in `master`
in order to keep the example code as simple as possible.
