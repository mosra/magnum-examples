#!/usr/bin/env python3

#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
#           Vladimír Vondruš <mosra@centrum.cz>
#
#   This is free and unencumbered software released into the public domain.
#
#   Anyone is free to copy, modify, publish, use, compile, sell, or distribute
#   this software, either in source code form or as a compiled binary, for any
#   purpose, commercial or non-commercial, and by any means.
#
#   In jurisdictions that recognize copyright laws, the author or authors of
#   this software dedicate any and all copyright interest in the software to
#   the public domain. We make this dedication for the benefit of the public
#   at large and to the detriment of our heirs and successors. We intend this
#   dedication to be an overt act of relinquishment in perpetuity of all
#   present and future rights to this software under copyright law.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

import array

from magnum import *
from magnum import gl, platform, shaders

class TriangleExample(platform.Application):
    def __init__(self):
        configuration = self.Configuration()
        configuration.title = "Magnum Python Triangle Example"
        platform.Application.__init__(self, configuration)

        buffer = gl.Buffer()
        buffer.set_data(array.array('f', [
            -0.5, -0.5, 1.0, 0.0, 0.0,
             0.5, -0.5, 0.0, 1.0, 0.0,
             0.0,  0.5, 0.0, 0.0, 1.0
        ]))

        self._mesh = gl.Mesh()
        self._mesh.count = 3
        self._mesh.add_vertex_buffer(buffer, 0, 5*4,
            shaders.VertexColor2D.POSITION)
        self._mesh.add_vertex_buffer(buffer, 2*4, 5*4,
            shaders.VertexColor2D.COLOR3)

        self._shader = shaders.VertexColor2D()

    def draw_event(self):
        gl.default_framebuffer.clear(gl.FramebufferClear.COLOR)

        self._mesh.draw(self._shader)
        self.swap_buffers()

exit(TriangleExample().exec())
