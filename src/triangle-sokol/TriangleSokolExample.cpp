/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Magnum/Math/Color.h>
#include <Magnum/Platform/Sdl2Application.h>

#include "flextGL.h"

/* Integrate with Magnum a bit */
#define SOKOL_ASSERT(c) CORRADE_INTERNAL_ASSERT(c)
#define SOKOL_LOG(c) do { Corrade::Utility::Debug{} << c; } while(0)
#define SOKOL_UNREACHABLE CORRADE_ASSERT_UNREACHABLE()
#define SOKOL_GLCORE33
#include "sokol_gfx.h"

namespace Magnum { namespace Examples {

using namespace Math::Literals;

class TriangleSokolExample: public Platform::Application {
    public:
        explicit TriangleSokolExample(const Arguments& arguments);
        ~TriangleSokolExample();

    private:
        void drawEvent() override;

        SDL_GLContext _context;

        sg_buffer _vertices;
        sg_shader _shader;
        sg_pipeline _pipeline;
};

TriangleSokolExample::TriangleSokolExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("Magnum Triangle using sokol_gfx")
        .setWindowFlags(Configuration::WindowFlag::Contextless|Configuration::WindowFlag::OpenGL)}
{
    /* Initialize context using toolkit-specific functionality. When the
       Magnum::GL library is not used, this is left completely to the user ---
       some renderers may have their own routines, some expect the user to do
       the initialization. OpenGL is used only as an example, could be anything
       else, Vulkan, D3D, Metal. */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    _context = SDL_GL_CreateContext(window());
    if(!_context) Fatal{} << "Can not create context:" << SDL_GetError();
    SDL_GL_MakeCurrent(window(), _context);

    flextInit();

    /* Setup sokol_gfx */
    {
        sg_desc desc{};
        sg_setup(&desc);
    }

    /* A vertex buffer */
    {
        const struct TriangleVertex {
            Vector2 position;
            Color3 color;
        } data[]{
            {{-0.5f, -0.5f}, 0xff0000_rgbf}, /* Left vertex, red color */
            {{ 0.5f, -0.5f}, 0x00ff00_rgbf}, /* Right vertex, green color */
            {{ 0.0f,  0.5f}, 0x0000ff_rgbf}  /* Top vertex, blue color */
        };
        sg_buffer_desc desc{};
        desc.content = data;
        desc.size = sizeof(data);
        _vertices = sg_make_buffer(&desc);
    }

    /* A shader */
    {
        sg_shader_desc desc{};
        desc.vs.source = "#version 330\n#line " CORRADE_LINE_STRING "\n" R"GLSL(
in vec4 position;
in vec4 color;
out vec4 interpolatedColor;

void main() {
    gl_Position = position;
    interpolatedColor = color;
}
)GLSL";
        desc.fs.source = "#version 330\n#line " CORRADE_LINE_STRING "\n" R"GLSL(
in vec4 interpolatedColor;
out vec4 fragmentColor;

void main() {
    fragmentColor = interpolatedColor;
}
)GLSL";
        _shader = sg_make_shader(&desc);
    }

    /* A pipeline state object */
    {
        sg_pipeline_desc desc{};
        desc.shader = _shader;
        desc.layout.attrs[0].name = "position";
        desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
        desc.layout.attrs[1].name = "color";
        desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3;
        _pipeline = sg_make_pipeline(&desc);
    }
}

TriangleSokolExample::~TriangleSokolExample() {
    sg_shutdown();

    SDL_GL_DeleteContext(_context);
}

void TriangleSokolExample::drawEvent() {
    /* Clear the framebuffer */
    {
        sg_pass_action action{};
        action.colors[0].action = SG_ACTION_CLEAR;
        Color4::from(action.colors[0].val) = 0x1f1f1f_rgbf;
        sg_begin_default_pass(&action, framebufferSize().x(), framebufferSize().y());
    }

    /* Draw the triangle */
    {
        sg_draw_state state{};
        state.pipeline = _pipeline;
        state.vertex_buffers[0] = _vertices;
        sg_apply_draw_state(&state);
        sg_draw(0, 3, 1);
    }

    sg_end_pass();
    sg_commit();

    swapBuffers();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::TriangleSokolExample)
