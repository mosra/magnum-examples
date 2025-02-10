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
import array

from magnum import *
from magnum import gl, shaders, trade
from magnum.platform.sdl2 import Application

class TexturedQuadShader(gl.AbstractShaderProgram):
    POSITION = gl.Attribute(
        gl.Attribute.Kind.GENERIC, 0,
        gl.Attribute.Components.TWO,
        gl.Attribute.DataType.FLOAT)
    TEXTURE_COORDINATES = gl.Attribute(
        gl.Attribute.Kind.GENERIC, 1,
        gl.Attribute.Components.TWO,
        gl.Attribute.DataType.FLOAT)

    _texture_unit = 0

    def __init__(self):
        super().__init__()

        vert = gl.Shader(gl.Version.GL330, gl.Shader.Type.VERTEX)
        vert.add_source("""
layout(location = 0) in vec4 position;
layout(location = 1) in vec2 textureCoordinates;

out vec2 interpolatedTextureCoordinates;

void main() {
    interpolatedTextureCoordinates = textureCoordinates;

    gl_Position = position;
}
""".lstrip())
        vert.compile()
        self.attach_shader(vert)

        frag = gl.Shader(gl.Version.GL330, gl.Shader.Type.FRAGMENT)
        frag.add_source("""
uniform vec3 color = vec3(1.0, 1.0, 1.0);
uniform sampler2D textureData;

in vec2 interpolatedTextureCoordinates;

out vec4 fragmentColor;

void main() {
    fragmentColor.rgb = color*texture(textureData, interpolatedTextureCoordinates).rgb;
    fragmentColor.a = 1.0;
}
""".lstrip())
        frag.compile()
        self.attach_shader(frag)

        self.link()

        self._color_uniform = self.uniform_location('color')
        self.set_uniform(self.uniform_location('textureData'), self._texture_unit)

    def color(self, color: Color3):
        self.set_uniform(self._color_uniform, color)
    color = property(None, color)

    def bind_texture(self, texture: gl.Texture2D):
        texture.bind(self._texture_unit)

class TexturedQuadExample(Application):
    def __init__(self):
        configuration = self.Configuration()
        configuration.title = "Magnum Python Textured Quad Example"
        Application.__init__(self, configuration)

        vertices = gl.Buffer()
        vertices.set_data(array.array('f', [
             0.5, -0.5, 1.0, 0.0,
             0.5,  0.5, 1.0, 1.0,
            -0.5, -0.5, 0.0, 0.0,
            -0.5,  0.5, 0.0, 1.0
        ]))
        indices = gl.Buffer()
        indices.set_data(array.array('I', [
            0, 1, 2,
            2, 1, 3
        ]))

        self._mesh = gl.Mesh()
        self._mesh.count = 6
        self._mesh.add_vertex_buffer(vertices, 0, 4*4,
            TexturedQuadShader.POSITION)
        self._mesh.add_vertex_buffer(vertices, 2*4, 4*4,
            TexturedQuadShader.TEXTURE_COORDINATES)
        self._mesh.set_index_buffer(indices, 0, gl.MeshIndexType.UNSIGNED_INT)

        importer = trade.ImporterManager().load_and_instantiate('TgaImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__),
                                        '../texturedquad/stone.tga'))
        image = importer.image2d(0)

        self._texture = gl.Texture2D()
        self._texture.wrapping = gl.SamplerWrapping.CLAMP_TO_EDGE
        self._texture.minification_filter = gl.SamplerFilter.LINEAR
        self._texture.magnification_filter = gl.SamplerFilter.LINEAR
        self._texture.set_storage(1, gl.TextureFormat.RGB8, image.size)
        self._texture.set_sub_image(0, Vector2i(), image)

        # or self._shader = shaders.FlatGL2D(shaders.FlatGL2D.Flags.TEXTURED)
        self._shader = TexturedQuadShader()

    def draw_event(self):
        gl.default_framebuffer.clear(gl.FramebufferClear.COLOR)

        self._shader.color = (1.0, 0.7, 0.7)
        self._shader.bind_texture(self._texture)
        self._shader.draw(self._mesh)

        self.swap_buffers()

exit(TexturedQuadExample().exec())
