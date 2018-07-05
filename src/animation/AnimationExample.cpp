/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016 —
            Vladimír Vondruš <mosra@centrum.cz>

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

#include <Magnum/Buffer.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Renderer.h>
#include <Magnum/Math/DualQuaternion.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData3D.h>

#include <vector>
#include <utility>

namespace Magnum { namespace Examples {

using namespace Magnum::Math::Literals;

class AnimationExample: public Platform::Application {
    public:
        explicit AnimationExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void setupCubeMesh();

        Buffer _indexBuffer, _vertexBuffer;
        Mesh _mesh;
        Shaders::Phong _shader;

        Matrix4 _transformation, _projection;
        Vector2i _previousMousePosition;

        /* Animation related members */
        Int _curFrame = 0;
        Int _curKeyFrame = 0;
        std::vector<std::pair<Int, DualQuaternion>> _animation;
};

AnimationExample::AnimationExample(const Arguments& arguments): Platform::Application{arguments, Configuration{}.setTitle("Magnum Animation Example")} {
    Renderer::enable(Renderer::Feature::DepthTest);
    Renderer::enable(Renderer::Feature::FaceCulling);

    setupCubeMesh();

    _projection = Matrix4::perspectiveProjection(35.0_degf, Vector2{defaultFramebuffer.viewport().size()}.aspectRatio(), 0.01f, 100.0f)*
                  Matrix4::translation(Vector3::zAxis(-10.0f));

    _animation.emplace_back(0, DualQuaternion::translation({0, 0, 0})*DualQuaternion::rotation(Deg(45.0f), {0, 1, 0}));
    _animation.emplace_back(60, DualQuaternion::translation({2, 2, 2})*DualQuaternion::rotation(Deg(0.0f), {0, 1, 0}));
    _animation.emplace_back(120, DualQuaternion::translation({0, 0, 0})*DualQuaternion::rotation(Deg(45.0f), {0, 1, 0}));
}

void AnimationExample::setupCubeMesh() {
    const Trade::MeshData3D cube = Primitives::Cube::solid();

    _vertexBuffer.setData(MeshTools::interleave(cube.positions(0), cube.normals(0)), BufferUsage::StaticDraw);

    Containers::Array<char> indexData;
    Mesh::IndexType indexType;
    UnsignedInt indexStart, indexEnd;
    std::tie(indexData, indexType, indexStart, indexEnd) = MeshTools::compressIndices(cube.indices());
    _indexBuffer.setData(indexData, BufferUsage::StaticDraw);

    _mesh.setPrimitive(cube.primitive())
        .setCount(cube.indices().size())
        .addVertexBuffer(_vertexBuffer, 0, Shaders::Phong::Position{}, Shaders::Phong::Normal{})
        .setIndexBuffer(_indexBuffer, 0, indexType, indexStart, indexEnd);
}

void AnimationExample::drawEvent() {
    /* Update animation */

    /* Check whether to increate keyframe counter */
    if(_curFrame > _animation[_curKeyFrame+1].first) {
        ++_curKeyFrame;
        if(_curKeyFrame + 1 >= _animation.size()) {
            _curKeyFrame = 0;
            _curFrame = 0;
        }
    }

    const auto lastFrame = _animation[_curKeyFrame];
    const auto nextFrame = _animation[_curKeyFrame+1];

    const DualQuaternion last = lastFrame.second;
    const DualQuaternion next = nextFrame.second;

    const Float factor = Float(_curFrame - lastFrame.first)/(nextFrame.first - lastFrame.first);

    const DualQuaternion interpolated = Math::sclerp(last, next, factor);
    _transformation = interpolated.toMatrix();

    defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);

    _shader.setLightPosition({7.0f, 5.0f, 2.5f})
        .setLightColor(Color3{1.0f})
        .setDiffuseColor(Color3{0.8f})
        .setAmbientColor(Color3{0.8f})
        .setTransformationMatrix(_transformation)
        .setNormalMatrix(_transformation.rotationScaling())
        .setProjectionMatrix(_projection);
    _mesh.draw(_shader);

    swapBuffers();
    ++_curFrame;

    redraw();
}

void AnimationExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector2 delta = 3.0f*
        Vector2{event.position() - _previousMousePosition}/
        Vector2{defaultFramebuffer.viewport().size()};

    _transformation =
        Matrix4::rotationX(Rad{delta.y()})*
        _transformation*
        Matrix4::rotationY(Rad{delta.x()});

    _previousMousePosition = event.position();
    event.setAccepted();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::AnimationExample)
