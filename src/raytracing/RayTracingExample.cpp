/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
            Vladimír Vondruš <mosra@centrum.cz>
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

#include "../arcball/ArcBall.h"
#include "RayTracer.h"

#include <Corrade/Containers/Pointer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Platform/Sdl2Application.h>

namespace Magnum { namespace Examples {
using namespace Math::Literals;

class RayTracingExample : public Platform::Application {
public:
    explicit RayTracingExample(const Arguments& arguments);

private:
    void drawEvent() override;
    void viewportEvent(ViewportEvent& event) override;
    void keyPressEvent(KeyEvent& event) override;
    void mousePressEvent(MouseEvent& event) override;
    void mouseMoveEvent(MouseMoveEvent& event) override;
    void mouseScrollEvent(MouseScrollEvent& event) override;

    void renderAndUpdateBlockPixels();
    void resizeBuffers(const Vector2i& bufferSize);
    void updateRayTracerCamera();

    Containers::Pointer<ArcBall> _arcballCamera;
    GL::Texture2D                _texBuffer{ NoCreate };
    GL::Framebuffer              _frameBuffer{ NoCreate };

    Containers::Pointer<RayTracer> _rayTracer;
    bool _bPaused { false };
};

RayTracingExample::RayTracingExample(const Arguments& arguments) :
    Platform::Application{arguments, NoCreate} {
    /* Setup window */
    {
        const Vector2 dpiScaling = this->dpiScaling({});
        Configuration conf;
        conf.setTitle("Magnum Ray Tracing Example")
            .setSize(conf.size(), dpiScaling)
            .setWindowFlags(Configuration::WindowFlag::Resizable);
        create(conf, GLConfiguration{});
    }

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    /* Set up the camera and ray tracer */
    {
        const Vector3 eye        = Vector3(5.f, 1.f, 5.5f);
        const Vector3 viewCenter = Vector3(1.f, 0.5f, 0.f);
        const Vector3 up         = Vector3(0, 1, 0);
        const Deg     fov { 45.0_degf };
        _arcballCamera.reset(new ArcBall(eye, viewCenter, up, fov, windowSize()));
        _rayTracer.reset(new RayTracer(eye, viewCenter, up, fov, Vector2{ framebufferSize() }.aspectRatio(),
                                       framebufferSize()));
        resizeBuffers(framebufferSize());
    }

    /* Start the timer, loop frame as fast as possible*/
    setSwapInterval(0);
    setMinimalLoopPeriod(0);
}

void RayTracingExample::drawEvent() {
    if(_bPaused) {
        return;
    }

    /* Call arcball update in every frame
     * This will do nothing if the camera has not been changed
     * Otherwise, camera transformation will be propagated into the ray tracer
     */
    if(_arcballCamera->updateTransformation()) {
        updateRayTracerCamera();
    }

    /* Render a block of pixels */
    renderAndUpdateBlockPixels();
    swapBuffers();
    redraw();
}

void RayTracingExample::renderAndUpdateBlockPixels() {
    _rayTracer->renderBlock();
    const auto& pixels = _rayTracer->renderedBuffer();
    const auto  image  = ImageView2D(PixelFormat::RGBA8Unorm, framebufferSize(), pixels);
    _texBuffer.setSubImage(0, {}, image);
    GL::AbstractFramebuffer::blit(_frameBuffer, GL::defaultFramebuffer, _frameBuffer.viewport(), GL::FramebufferBlit::Color);
}

void RayTracingExample::viewportEvent(ViewportEvent& event) {
    const auto newBufferSize = event.framebufferSize();
    GL::defaultFramebuffer.setViewport({ {}, newBufferSize });
    resizeBuffers(newBufferSize);
    _rayTracer->resizeBuffers(newBufferSize);
    _arcballCamera->reshape(windowSize());
    updateRayTracerCamera();
}

void RayTracingExample::keyPressEvent(KeyEvent& event) {
    switch(event.key()) {
        case KeyEvent::Key::Plus:
        case KeyEvent::Key::NumAdd:
            _arcballCamera->zoom(0.5f);
            break;
        case KeyEvent::Key::Minus:
        case KeyEvent::Key::NumSubtract:
            _arcballCamera->zoom(-0.5f);
            break;
        case KeyEvent::Key::Left:
            _arcballCamera->translateDelta(Vector2{ -0.05f, 0 });
            break;
        case KeyEvent::Key::Right:
            _arcballCamera->translateDelta(Vector2{ 0.05f, 0 });
            break;
        case KeyEvent::Key::Up:
            _arcballCamera->translateDelta(Vector2{ 0, 0.05f });
            break;
        case KeyEvent::Key::Down:
            _arcballCamera->translateDelta(Vector2{ 0, -0.05f });
            break;

        case KeyEvent::Key::R:
            _arcballCamera->reset();
            break;

        case KeyEvent::Key::M:
            _rayTracer->markNextBlock() ^= true;
            break;

        case KeyEvent::Key::N:
            _rayTracer->generateSceneObjects();
            _rayTracer->clearBuffers();
            break;

        case KeyEvent::Key::Space:
            _bPaused ^= true;
            break;

        default:
            return;
    }
    event.setAccepted(true);
    redraw(); /* camera has changed, or ray tracer started/paused, redraw! */
}

void RayTracingExample::mousePressEvent(MouseEvent& event) {
    if(!(event.button() == MouseEvent::Button::Left ||
         event.button() == MouseEvent::Button::Right)) { return; }
    _arcballCamera->initTransformation(event.position());
    event.setAccepted();
}

void RayTracingExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left ||
         event.buttons() & MouseMoveEvent::Button::Right)) { return; }

    if(event.buttons() & MouseMoveEvent::Button::Left) {
        _arcballCamera->rotate(event.position());
    } else {
        _arcballCamera->translate(event.position());
    }
    event.setAccepted();
    redraw(); /* camera has changed, redraw! */
}

void RayTracingExample::mouseScrollEvent(MouseScrollEvent& event) {
    const Float delta = event.offset().y();
    if(Math::abs(delta) < 1.0e-2f) {
        return;
    }
    _arcballCamera->zoom(delta);
    event.setAccepted();
    redraw(); /* camera has changed, redraw! */
}

void RayTracingExample::resizeBuffers(const Vector2i& bufferSize) {
    _texBuffer = GL::Texture2D();
    _texBuffer.setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear, GL::SamplerMipmap::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMaxAnisotropy(GL::Sampler::maxMaxAnisotropy())
        .setStorage(1, GL::TextureFormat::RGBA8, bufferSize);

    _frameBuffer = GL::Framebuffer(GL::defaultFramebuffer.viewport());
    _frameBuffer.attachTexture(GL::Framebuffer::ColorAttachment{ 0 }, _texBuffer, 0);
}

void RayTracingExample::updateRayTracerCamera() {
    const Matrix4& transformation = _arcballCamera->transformationMatrix();
    const Vector3  eye        = transformation.translation();
    const Vector3  viewCenter = transformation.translation() - transformation.backward() * _arcballCamera->viewDistance();
    const Vector3  up         = transformation.up();
    const Deg      fov { 45.0_degf };
    _rayTracer->setViewParameters(eye, viewCenter, up, fov, Vector2{ framebufferSize() }.aspectRatio());
}
} }

MAGNUM_APPLICATION_MAIN(Magnum::Examples::RayTracingExample)
