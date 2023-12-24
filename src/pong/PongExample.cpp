/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023 — Vladimír Vondruš <mosra@centrum.cz>
        2023 — Pablo Duboue <pablo.duboue@gmail.com>

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

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/VertexColorGL.h>

#include <iostream>

#include "Paddle.h"
#include "Ball.h"


namespace Magnum { namespace Examples {

using namespace Math::Literals;

class PongExample: public Platform::Application {
    public:
        explicit PongExample(const Arguments& arguments);
        ~PongExample();

    private:
        void drawEvent() override;
        void tickEvent() override;
        void keyPressEvent(KeyEvent& event) override;

        GL::Mesh _mesh;
        Shaders::VertexColorGL2D _shader;

        Paddle* _paddle1;
        Paddle* _paddle2;
        Ball* _ball;
        bool _running;

        int _score[2];
};

PongExample::PongExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("Magnum Pong Example")}
{
    _paddle1 = new Paddle(-0.968f, 0.0f, true);
    _paddle2 = new Paddle( 0.968f, 0.0f, false);
    _ball = new Ball(0.0f, 0.0f);
    _running = true;
    _score[0] = _score[1] = 0;

    setMinimalLoopPeriod(2);
}

PongExample::~PongExample() {
    delete _paddle1;
    delete _paddle2;
    delete _ball;
}

void PongExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _paddle1->draw();
    _paddle2->draw();
    _ball->draw();
    
    swapBuffers();
}

void PongExample::tickEvent() {
    _paddle1->update(_ball);
    _paddle2->update(_ball);
    
    if (_ball->x() < -1.0f) {
      _score[1]++;
      _ball->start();
    }
    if (_ball->x() > 1.0f) {
      _score[0]++;
      _ball->start();
    }    
    _ball->update();
    redraw();
}

void PongExample::keyPressEvent(KeyEvent& event) {
     if(event.key() == KeyEvent::Key::Esc) {
         exit(0);
     }
     _paddle1->handleEvent(event);
     _paddle2->handleEvent(event);
     redraw();
}


}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::PongExample)
