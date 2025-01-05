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
from magnum import gl, meshtools, scenegraph, shaders, trade
from magnum.platform.sdl2 import Application
from magnum.scenegraph.matrix import Scene3D, Object3D

class ColoredDrawable(scenegraph.Drawable3D):
    def __init__(self, object: Object3D, drawables: scenegraph.DrawableGroup3D,
                 mesh: gl.Mesh, shader: shaders.PhongGL, color: Color4):
        scenegraph.Drawable3D.__init__(self, object, drawables)

        self._mesh = mesh
        self._shader = shader
        self._color = color

    def draw(self, transformation_matrix: Matrix4, camera: scenegraph.Camera3D):
        self._shader.light_positions = [
            Vector4(camera.camera_matrix.transform_point((-3.0, 10.0, 10.0)), 0.0)
        ]
        self._shader.diffuse_color = self._color
        self._shader.transformation_matrix = transformation_matrix
        self._shader.normal_matrix = transformation_matrix.rotation_scaling()
        self._shader.projection_matrix = camera.projection_matrix
        self._shader.draw(self._mesh)

class ViewerExample(Application):
    def __init__(self):
        configuration = self.Configuration()
        configuration.title = "Magnum Python Viewer Example"
        Application.__init__(self, configuration)

        gl.Renderer.enable(gl.Renderer.Feature.DEPTH_TEST)
        gl.Renderer.enable(gl.Renderer.Feature.FACE_CULLING)

        # Scene and drawables
        self._scene = Scene3D()
        self._drawables = scenegraph.DrawableGroup3D()

        # Every scene needs a camera
        camera_object = Object3D(parent=self._scene)
        camera_object.translate(Vector3.z_axis(5.0))
        self._camera = scenegraph.Camera3D(camera_object)
        self._camera.aspect_ratio_policy = scenegraph.AspectRatioPolicy.EXTEND
        self._camera.projection_matrix = Matrix4.perspective_projection(
            fov=Deg(35.0), aspect_ratio=1.0, near=0.01, far=100.0)
        self._camera.viewport = self.framebuffer_size

        # Base object, parent of all (for easy manipulation)
        self._manipulator = Object3D(parent=self._scene)

        # Setup renderer and shader defaults
        gl.Renderer.enable(gl.Renderer.Feature.DEPTH_TEST)
        gl.Renderer.enable(gl.Renderer.Feature.FACE_CULLING)
        colored_shader = shaders.PhongGL()
        colored_shader.ambient_color = Color3(0.06667)
        colored_shader.shininess = 80.0

        # Import Suzanne head and eyes (yes, sorry, it's all hardcoded here)
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__),
                                        '../viewer/scene.glb'))
        suzanne_object = Object3D(parent=self._manipulator)
        suzanne_mesh = meshtools.compile(
            importer.mesh(importer.mesh_for_name('Suzanne')))
        suzanne_eyes_mesh = meshtools.compile(
            importer.mesh(importer.mesh_for_name('Eyes')))
        self._suzanne = ColoredDrawable(suzanne_object, self._drawables,
            suzanne_mesh, colored_shader, Color3(0.15, 0.49, 1.0))
        self._suzanne_eyes = ColoredDrawable(suzanne_object, self._drawables,
            suzanne_eyes_mesh, colored_shader, Color3(0.95))

    def draw_event(self):
        gl.default_framebuffer.clear(gl.FramebufferClear.COLOR|
                                     gl.FramebufferClear.DEPTH)

        self._camera.draw(self._drawables)
        self.swap_buffers()

    def pointer_move_event(self, event: Application.PointerMoveEvent):
        if not event.is_primary or \
           not (event.pointers & (self.Pointer.MOUSE_LEFT|self.Pointer.FINGER)):
            return

        delta = 1.0*event.relative_position/Vector2(self.window_size)
        self._manipulator.rotate_y_local(Rad(delta.x))
        self._manipulator.rotate_x(Rad(delta.y))
        self.redraw()

    def scroll_event(self, event: Application.ScrollEvent):
        if not event.offset.y: return

        # Distance to origin
        distance = self._camera.object.transformation.translation.z

        # Move 15% of the distance back or forward
        self._camera.object.translate(Vector3.z_axis(
            distance*(1.0 - (1.0/0.85 if event.offset.y > 0 else 0.85))))

        self.redraw()

exit(ViewerExample().exec())
