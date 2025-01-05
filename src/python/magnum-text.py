#!/usr/bin/env python3

#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#       2020, 2021, 2022, 2023, 2024, 2025
#            — Vladimír Vondruš <mosra@centrum.cz>
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

import os

from magnum import *
from magnum import gl, shaders, text
from magnum.platform.sdl2 import Application

class TextExample(Application):
    def __init__(self):
        configuration = self.Configuration()
        configuration.window_flags |= self.Configuration.WindowFlag.RESIZABLE
        configuration.title = "Magnum Python Text Example"
        Application.__init__(self, configuration)

        # Load a TrueTypeFont plugin and open the font
        self._font = text.FontManager().load_and_instantiate('TrueTypeFont')
        self._font.open_file(os.path.join(os.path.dirname(__file__),
            '../text/SourceSansPro-Regular.ttf'), 180.0)

        # Glyphs we need to render everything
        self._cache = text.DistanceFieldGlyphCacheGL(Vector2i(2048), Vector2i(512), 22)
        self._font.fill_glyph_cache(self._cache,
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789:-+,.!°ěäПривітСΓειασουκόμ ")

        # Text that rotates using mouse wheel. Size relative to the window size
        # (1/10 of it) -- if you resize the window, it gets bigger
        self._rotating_text = text.Renderer2D(self._font, self._cache, 0.2,
                                              text.Alignment.MIDDLE_CENTER)
        self._rotating_text.reserve(128)
        self._rotating_text.render(
            "Hello, world!\n"
            "Ahoj, světe!\n"
            "Привіт Світ!\n"
            "Γεια σου κόσμε!\n"
            "Hej Världen!")

        # Dynamically updated text that shows rotation/zoom of the other. Size
        # in points that stays the same if you resize the window. Aligned so
        # top right of the bounding box is at mesh origin, and then transformed
        # so the origin is at the top right corner of the window.
        self._dynamic_text = text.Renderer2D(self._font, self._cache, 32.0,
                                             text.Alignment.TOP_RIGHT)
        self._dynamic_text.reserve(40)
        self._transformation_projection_dynamic_text =\
            Matrix3.projection(Vector2(self.window_size))@\
            Matrix3.translation(Vector2(self.window_size)*0.5)

        self._transformation_rotating_text = Matrix3.rotation(Deg(-10.0))
        self._projection_rotating_text = Matrix3.projection(
            Vector2.x_scale(Vector2(self.window_size).aspect_ratio()))

        self._shader = shaders.DistanceFieldVectorGL2D()

        gl.Renderer.enable(gl.Renderer.Feature.BLENDING)
        gl.Renderer.set_blend_function(gl.Renderer.BlendFunction.ONE,
            gl.Renderer.BlendFunction.ONE_MINUS_SOURCE_ALPHA)
        gl.Renderer.set_blend_equation(gl.Renderer.BlendEquation.ADD,
            gl.Renderer.BlendEquation.ADD)

        self.update_text()

    def draw_event(self):
        gl.default_framebuffer.clear(gl.FramebufferClear.COLOR|
                                     gl.FramebufferClear.DEPTH)

        self._shader.bind_vector_texture(self._cache.texture)

        self._shader.transformation_projection_matrix = \
            self._projection_rotating_text @ self._transformation_rotating_text
        self._shader.color = [0.184, 0.514, 0.8]
        self._shader.outline_color = [0.863, 0.863, 0.863]
        self._shader.outline_range = (0.45, 0.35)
        self._shader.smoothness = 0.025/\
            self._transformation_rotating_text.uniform_scaling()
        self._shader.draw(self._rotating_text.mesh)

        self._shader.transformation_projection_matrix = \
            self._transformation_projection_dynamic_text
        self._shader.color = [1.0, 1.0, 1.0]
        self._shader.outline_range = (0.5, 1.0)
        self._shader.smoothness = 0.075
        self._shader.draw(self._dynamic_text.mesh)

        self.swap_buffers()

    def viewport_event(self, event: Application.ViewportEvent):
        gl.default_framebuffer.viewport = ((Vector2i(), event.framebuffer_size))

        self._projection_rotating_text = Matrix3.projection(
            Vector2.x_scale(Vector2(self.window_size).aspect_ratio()))
        self._transformation_projection_dynamic_text =\
            Matrix3.projection(Vector2(self.window_size))@\
            Matrix3.translation(Vector2(self.window_size)*0.5)

    def scroll_event(self, event: Application.ScrollEvent):
        if not event.offset.y: return

        if event.offset.y > 0:
            self._transformation_rotating_text =\
                Matrix3.rotation(Deg(1.0)) @\
                Matrix3.scaling(Vector2(1.1)) @\
                self._transformation_rotating_text
        else:
            self._transformation_rotating_text =\
                Matrix3.rotation(Deg(-1.0)) @\
                Matrix3.scaling(Vector2(1.0/1.1)) @\
                self._transformation_rotating_text

        self.update_text()

        event.accepted = True
        self.redraw()

    def update_text(self):
        # TODO show rotation once Complex.from_matrix() is a thing
        self._dynamic_text.render("Scale: {:.2}"
            .format(self._transformation_rotating_text.uniform_scaling()))

exit(TextExample().exec())
