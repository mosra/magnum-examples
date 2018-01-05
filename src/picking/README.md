This example demonstrates usage of multiple framebuffer attachments to
implement object picking. One attachment is used for color output, the other
contains object IDs. The color buffer is blit to window framebuffer, a pixel
from the other is read after mouse click to retrieve object ID. Requires OpenGL
3.3+.

![Object picking](picking.png)

Key controls
------------

Use **mouse drag** to rotate the scene, **mouse click** to highlight particular
object.

Ports
-----

The [ports branch](https://github.com/mosra/magnum-examples/tree/ports/src/picking)
contains additional patches for Emscripten support that aren't present in
`master` in order to keep the example code as simple as possible.
