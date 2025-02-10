/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
        2018 — Jonathan Hale <squareys@googlemail.com>

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

#include <Magnum/Magnum.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cylinder.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Trade/MeshData.h>

#include <Leap.h>

namespace Magnum { namespace Examples {

using namespace Math::Literals;

class LeapMotionExample: public Platform::Application {
    public:
        explicit LeapMotionExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void drawBone(const Leap::Bone& bone, bool start, bool end, const Color3& color);

        GL::Mesh _cylinder{NoCreate};
        GL::Mesh _sphere{NoCreate};

        Shaders::PhongGL _shader;

        const Matrix4 _projectionMatrix = Matrix4::perspectiveProjection(90.0_degf, 3.0f/2.0f, 0.01f, 25.0f);
        const Matrix4 _viewMatrix = Matrix4::translation({0.0f, -3.0f, -5.0f});

        /* LeapMotion specific members */
        Leap::Controller _controller;
};

LeapMotionExample::LeapMotionExample(const Arguments& arguments):
    Platform::Application(arguments, Configuration{}.setTitle("Magnum Leap Motion Example"), GLConfiguration{}.setSampleCount(16))
{
    /* To use LeapMotion with VR, you should set
       _controller.setPolicy(Leap::Controller::PolicyFlag::POLICY_OPTIMIZE_HMD); */

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

    /* As we sample the Leap controller every frame, enabling vsyc will cause more latency,
       but seems to produce more stable results.
       To reduce latency and fully utlize the ~115 fps hand tracking, disable vsync. */
    setSwapInterval(1);

    /* Setup cylinder and sphere meshes */
    _cylinder = MeshTools::compile(Primitives::cylinderSolid(2, 16, 0.5f));
    _sphere = MeshTools::compile(Primitives::uvSphereSolid(16, 16));

    /* Setup shader */
    _shader.setSpecularColor(Color3(1.0f))
           .setShininess(20)
           .setLightPositions({{0.0f, 5.0f, 5.0f, 0.0f}});
}

void LeapMotionExample::drawEvent() {
    /* Poll leap controller */
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    /* Render scene */
    if(_controller.isConnected()) {
        const Matrix4 viewProjMatrix = _projectionMatrix*_viewMatrix;
        _shader.setProjectionMatrix(viewProjMatrix);

        Leap::Frame frame = _controller.frame();
        for(const Leap::Hand& hand : frame.hands()) {
            const Color3 handColor = hand.isRight() ? Color3{1.0f, 0.0f, 0.0f} : Color3{0.0f, 1.0f, 1.0f};

            for(const Leap::Finger& finger : hand.fingers()) {
                for(int b = 0; b < 4; ++b) {
                    /* Leave out first bones of ring and middle finger, looks better */
                    if(b == 0 && (finger.type() == Leap::Finger::Type::TYPE_MIDDLE || finger.type() == Leap::Finger::Type::TYPE_RING)) continue;

                    const auto& bone = finger.bone(Leap::Bone::Type(b));
                    /* Only draw end on last bones */
                    drawBone(bone, true, b == 3, handColor);
                }
            }
        }
    }

    swapBuffers();
    redraw();
}

void LeapMotionExample::drawBone(const Leap::Bone& bone, bool start, bool end, const Color3& color) {
    const Vector3 boneCenter = 0.01f*Vector3::from(bone.center().toFloatPointer());
    const Vector3 boneDirection = 0.01f*Vector3::from(bone.direction().toFloatPointer());
    const Float boneLength = 0.01f*bone.length();

    const Matrix4 rotX = Matrix4::rotationX(90.0_degf);
    /* Draw cylinder */
    {
        const Matrix4 transformation = Matrix4::translation(boneCenter)*Matrix4::from(bone.basis().toArray4x4())*rotX*Matrix4::scaling({0.06f, boneLength, 0.06f});
        _shader.setDiffuseColor(Color3{1.0f, 1.0f, 1.0f})
               .setTransformationMatrix(transformation)
               .setNormalMatrix(transformation.normalMatrix())
               .draw(_cylinder);
    }

    /* Draw sphere at start of the bone */
    if(start) {
        const Vector3 prevJoint = 0.01f*Vector3::from(bone.prevJoint().toFloatPointer());
        const Matrix4 transformation = Matrix4::translation(prevJoint)*rotX*Matrix4::scaling(Vector3{0.08f});
        _shader.setDiffuseColor(color)
               .setTransformationMatrix(transformation)
               .setNormalMatrix(transformation.normalMatrix())
               .draw(_sphere);
    }

    /* Draw sphere at end of the bone */
    if(end) {
        const Vector3 nextJoint = 0.01f*Vector3::from(bone.nextJoint().toFloatPointer());
        const Matrix4 transformation = Matrix4::translation(nextJoint)*rotX*Matrix4::scaling(Vector3{0.08f});
        _shader.setDiffuseColor(color)
               .setTransformationMatrix(transformation)
               .setNormalMatrix(transformation.normalMatrix())
               .draw(_sphere);
    }
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::LeapMotionExample)
