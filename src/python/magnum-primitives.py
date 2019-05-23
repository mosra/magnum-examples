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

from magnum import *
from magnum import gl, meshtools, platform, primitives, shaders

class PrimitivesExample(platform.Application):
    def __init__(self):
        configuration = self.Configuration()
        configuration.title = "Magnum Python Primitives Example"
        platform.Application.__init__(self, configuration)

        gl.Renderer.enable(gl.Renderer.Feature.DEPTH_TEST)
        gl.Renderer.enable(gl.Renderer.Feature.FACE_CULLING)

        self._mesh = meshtools.compile(primitives.cube_solid())
        self._shader = shaders.Phong()

        self._transformation = (
            Matrix4.rotation_x(Deg(30.0))@
            Matrix4.rotation_y(Deg(40.0)))
        self._projection = (
            Matrix4.perspective_projection(
                fov=Deg(35.0), aspect_ratio=1.33333, near=0.01, far=100.0)@
            Matrix4.translation(Vector3.z_axis(-10.0)))
        self._color = Color3.from_hsv(Deg(35.0), 1.0, 1.0)
        self._previous_mouse_position = Vector2i()

    def draw_event(self):
        gl.default_framebuffer.clear(gl.FramebufferClear.COLOR|
                                     gl.FramebufferClear.DEPTH)

        self._shader.light_positions = [(7.0, 5.0, 2.5)]
        self._shader.light_colors = [Color3(1.0)]
        self._shader.diffuse_color = self._color
        self._shader.ambient_color = Color3.from_hsv(self._color.hue(), 1.0, 0.3)
        self._shader.transformation_matrix = self._transformation
        self._shader.normal_matrix = self._transformation.rotation_scaling()
        self._shader.projection_matrix = self._projection

        self._mesh.draw(self._shader)
        self.swap_buffers()

    def mouse_release_event(self, event: platform.Application.MouseEvent):
        self._color = Color3.from_hsv(self._color.hue() + Deg(50.0), 1.0, 1.0)
        self.redraw()

    def mouse_move_event(self, event: platform.Application.MouseMoveEvent):
        if event.buttons & self.MouseMoveEvent.Buttons.LEFT:
            delta = 1.0*(
                Vector2(event.position - self._previous_mouse_position)/
                Vector2(self.window_size()))
            self._transformation = (
                Matrix4.rotation_x(Rad(delta.y))@
                self._transformation@
                Matrix4.rotation_y(Rad(delta.x)))
            self.redraw()

        self._previous_mouse_position = event.position

exit(PrimitivesExample().exec())
