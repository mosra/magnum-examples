/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
        2020 — Nghia Truong <nghiatruong.vn@gmail.com>

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

#include <Corrade/Containers/Pointer.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/FormatStl.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Primitives/Square.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Trade/MeshData.h>
#ifdef CORRADE_TARGET_EMSCRIPTEN
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif

#include "../arcball/ArcBall.h"
#include "RayTracer.h"

namespace Magnum { namespace Examples {

using namespace Math::Literals;

class RayTracingExample: public Platform::Application {
    public:
        explicit RayTracingExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void viewportEvent(ViewportEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void pointerPressEvent(PointerEvent& event) override;
        void pointerReleaseEvent(PointerEvent& event) override;
        void pointerMoveEvent(PointerMoveEvent& event) override;
        void scrollEvent(ScrollEvent& event) override;

        void renderAndUpdateBlockPixels();
        void resizeBuffers(const Vector2i& bufferSize);
        void updateRayTracerCamera();

        Containers::Pointer<ArcBall> _arcballCamera;
        GL::Texture2D _texture{NoCreate};
        Shaders::FlatGL2D _shader{NoCreate};
        GL::Mesh _square{NoCreate};

        Containers::Pointer<RayTracer> _rayTracer;
        bool _depthOfField = false;
        bool _paused = false;
};

RayTracingExample::RayTracingExample(const Arguments& arguments):
    Platform::Application{arguments, NoCreate}
{
    Utility::Arguments args;
    args.addOption("block-size", "64")
            .setHelp("block-size", "size of a block to render at a time", "PIXELS")
        .addOption("max-samples", "100")
            .setHelp("max-samples", "max samples per pixel", "COUNT")
        .addOption("max-ray-depth", "16")
            .setHelp("max-ray-depth", "max ray depth", "DEPTH")
        .addSkippedPrefix("magnum")
        .parse(arguments.argc, arguments.argv);

    /* Delayed context creation so the command-line help is displayed w/o
       the engine startup log */
    create(Configuration{}
        .setTitle("Magnum Ray Tracing Example")
        .setWindowFlags(Configuration::WindowFlag::Resizable));

    /* Set up the camera and ray tracer */
    {
        const Vector3 eye{5.0f, 1.0f, 5.5f};
        const Vector3 viewCenter{1.0f, 0.5f, 0.0f};
        const Vector3 up{0.0f, 1.0f, 0.0f};
        const Deg fov = 45.0_degf;
        _arcballCamera.emplace(eye, viewCenter, up, fov, windowSize());
        _rayTracer.emplace(eye, viewCenter, up, fov,
            Vector2{framebufferSize()}.aspectRatio(), 0.0f, framebufferSize(),
            args.value<UnsignedInt>("block-size"),
            args.value<UnsignedInt>("max-samples"),
            args.value<UnsignedInt>("max-ray-depth"));
        resizeBuffers(framebufferSize());
    }

    /* Set up a full-screen square to render. Usually we would just blit the
       texture to the default framebuffer, but framebuffer blit is not on WebGL
       1 and it seems silly to require WebGL 2 for something that's mostly
       software rendering. OTOH this adds a dependency on four new libs
       (MeshTools, Primitives, Shaders and Trade) which isn't exactly ideal
       either. */
    _shader = Shaders::FlatGL2D{Shaders::FlatGL2D::Configuration{}
        .setFlags(Shaders::FlatGL2D::Flag::Textured)};
    _square = MeshTools::compile(
        Primitives::squareSolid(Primitives::SquareFlag::TextureCoordinates));

    #ifndef CORRADE_TARGET_EMSCRIPTEN
    /* Loop frame as fast as possible */
    setSwapInterval(0);
    #endif
}

void RayTracingExample::drawEvent() {
    if(_paused) return;

    /* Call arcball update in every frame. This will do nothing if the camera
       has not been changed; otherwise camera transformation will be propagated
       into the ray tracer */
    if(_arcballCamera->updateTransformation())
        updateRayTracerCamera();

    /* Render a block of pixels */
    renderAndUpdateBlockPixels();
    swapBuffers();

    /* Draw again if the raytracer is not done with all samples yet */
    if(!_rayTracer->done()) redraw();
}

void RayTracingExample::renderAndUpdateBlockPixels() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    /* Update window title with current iteration index */
    if(_rayTracer->currentBlock() == Vector2i{}) setWindowTitle(
        Utility::formatString("Magnum Ray Tracing Example (iteration {})",
            _rayTracer->iteration() + 2));

    _rayTracer->renderBlock();
    const auto& pixels = _rayTracer->renderedBuffer();
    _texture.setSubImage(0, {},
        ImageView2D(PixelFormat::RGBA8Unorm, framebufferSize(), pixels));
    _shader
        .bindTexture(_texture)
        .draw(_square);
}

void RayTracingExample::viewportEvent(ViewportEvent& event) {
    const auto newBufferSize = event.framebufferSize();
    GL::defaultFramebuffer.setViewport({{}, newBufferSize});
    _arcballCamera->reshape(windowSize());
    _rayTracer->resizeBuffers(newBufferSize);
    resizeBuffers(newBufferSize);
    updateRayTracerCamera();
}

void RayTracingExample::keyPressEvent(KeyEvent& event) {
    switch(event.key()) {
        case Key::R:
            _arcballCamera->reset();
            Debug{} << "Reset camera";
            break;

        case Key::D:
            _depthOfField ^= true;
            updateRayTracerCamera();
            if(_depthOfField)
                Debug{} << "Depth-of-Field enabled";
            else
                Debug{} << "Depth-of-Field disabled";
            break;

        case Key::M:
            _rayTracer->markNextBlock() ^= true;
            break;

        case Key::N:
            _rayTracer->generateSceneObjects();
            _rayTracer->clearBuffers();
            break;

        case Key::Space:
            _paused ^= true;
            break;

        default:
            return;
    }

    event.setAccepted(true);
    redraw(); /* camera has changed, or ray tracer started/paused, redraw! */
}

void RayTracingExample::pointerPressEvent(PointerEvent& event) {
    if(!event.isPrimary() ||
       !(event.pointer() & (Pointer::MouseLeft|Pointer::Finger)))
        return;

    #ifndef CORRADE_TARGET_EMSCRIPTEN
    /* Enable mouse capture so the mouse can drag outside of the window */
    /** @todo replace once https://github.com/mosra/magnum/pull/419 is in */
    SDL_CaptureMouse(SDL_TRUE);
    #endif

    _arcballCamera->initTransformation(event.position());
    event.setAccepted();
}

void RayTracingExample::pointerReleaseEvent(PointerEvent& event) {
    if(!event.isPrimary() ||
       !(event.pointer() & (Pointer::MouseLeft|Pointer::Finger)))
        return;

    #ifndef CORRADE_TARGET_EMSCRIPTEN
    /* Disable mouse capture again */
    /** @todo replace once https://github.com/mosra/magnum/pull/419 is in */
    SDL_CaptureMouse(SDL_FALSE);
    #endif
}

void RayTracingExample::pointerMoveEvent(PointerMoveEvent& event) {
    if(!event.isPrimary() ||
       !(event.pointers() & (Pointer::MouseLeft|Pointer::Finger)))
        return;

    if(event.modifiers() & Modifier::Shift)
        _arcballCamera->translate(event.position());
    else _arcballCamera->rotate(event.position());

    event.setAccepted();
}

void RayTracingExample::scrollEvent(ScrollEvent& event) {
    const Float delta = event.offset().y();
    if(Math::abs(delta) < 1.0e-2f) return;

    _arcballCamera->zoom(delta);
    event.setAccepted();
}

void RayTracingExample::resizeBuffers(const Vector2i& bufferSize) {
    _texture = GL::Texture2D();
    _texture.setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1,
            #if !(defined(MAGNUM_TARGET_GLES2) && defined(MAGNUM_TARGET_WEBGL))
            GL::TextureFormat::RGBA8,
            #else
            GL::TextureFormat::RGBA,
            #endif
            bufferSize);
}

void RayTracingExample::updateRayTracerCamera() {
    const Matrix4& transformation = _arcballCamera->transformationMatrix();
    const Vector3 eye = transformation.translation();
    const Vector3 viewCenter = transformation.translation() -
        transformation.backward()*_arcballCamera->viewDistance();
    const Vector3 up = transformation.up();
    const Deg fov = 45.0_degf;
    _rayTracer->setViewParameters(eye, viewCenter, up, fov,
        Vector2{framebufferSize()}.aspectRatio(), _depthOfField ? 0.08f : 0.0f);
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::RayTracingExample)
