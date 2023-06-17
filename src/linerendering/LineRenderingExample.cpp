#include <Corrade/Containers/Pair.h>
#include <Magnum/DimensionTraits.h>
#include <Magnum/Timeline.h>
#include <Magnum/Animation/Easing.h>
#include <Magnum/Animation/Player.h>
#include <Magnum/Animation/Track.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/MeshTools/CompileLines.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Duplicate.h>
#include <Magnum/MeshTools/GenerateIndices.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Square.h>
#include <Magnum/Shaders/Line.h>
#include <Magnum/Shaders/LineGL.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Trade/MeshData.h>

#ifdef CORRADE_TARGET_EMSCRIPTEN
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif

using namespace Magnum;
using namespace Math::Literals;

namespace { // TODO put into magnum

class LineRenderingExample: public Platform::Application {
    public:
        explicit LineRenderingExample(const Arguments& arguments);

    private:
        void viewportEvent(ViewportEvent& event) override;
        void mousePressEvent(MouseEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void drawEvent() override;

        /* Meshes. Filled in the constructor. */
        GL::Mesh _line{NoCreate};
        GL::Mesh _joints{NoCreate};
        GL::Mesh _circle{NoCreate};
        GL::Mesh _square{NoCreate};
        GL::Mesh _cube{NoCreate};

        /* Animation timeline */
        Timeline _timeline;

        /* Square scaling animation track and result variable */
        const Animation::Track<Float, Vector2> _animationScalingTrack{{
            {0.0f, {1.0f, 1.0f}},
            {1.0f, {0.7f, 0.4f}},
            {2.0f, {0.4f, 0.7f}},
            {3.0f, {1.0f, 1.0f}},
        }, Animation::ease<Vector2, Math::lerp, Animation::Easing::quadraticInOut>()};
        Vector2 _animationScaling;

        /* Circle and square rotation animation track and result variable
           (yes, it rotates just halfway, but it isn't noticeable in the
           output) */
        const Animation::Track<Float, Deg> _animationRotationTrack{{
            {0.0f, 0.0_degf},
            {3.0f, 180.0_degf}
        }, Math::lerp};
        Deg _animationRotation;

        /* Cube rotation animation track and result variable, here it has to
           rotate full circle */
        const Animation::Track<Float, Deg> _animationCubeRotationTrack{{
            {0.0f, 0.0_degf},
            {1.5f, 180.0_degf},
            {3.0f, 360.0_degf},
        }, Math::lerp};
        Deg _animationCubeRotation;

        /* Circle and square animation line width track and result variable */
        const Animation::Track<Float, Float> _animationWidthTrack{{
            {0.0f, 10.0f},
            {1.25f, 30.0f},
            {1.75f, 30.0f},
            {3.0f, 10.0f}
        }, Animation::ease<Float, Math::lerp, Animation::Easing::elasticIn>()};
        Float _animationWidth;

        /* Animation player, set up in the constructor */
        Animation::Player<Float> _animationPlayer;

        /* Shader variants used */
        Shaders::LineGL2D _shaderButtCaps{Shaders::LineGL2D::Configuration{}
            .setCapStyle(Shaders::LineCapStyle::Butt)};
        Shaders::LineGL2D _shaderSquareCaps{Shaders::LineGL2D::Configuration{}
            .setCapStyle(Shaders::LineCapStyle::Square)};
        Shaders::LineGL2D _shaderRoundCaps{Shaders::LineGL2D::Configuration{}
            .setCapStyle(Shaders::LineCapStyle::Round)};
        Shaders::LineGL2D _shaderTriangleCaps{Shaders::LineGL2D::Configuration{}
            .setCapStyle(Shaders::LineCapStyle::Triangle)};
        Shaders::LineGL3D _shaderRoundCaps3D{Shaders::LineGL3D::Configuration{}
            .setCapStyle(Shaders::LineCapStyle::Round)};

        /* Flat shader to show fallback to regular GL lines */
        Shaders::FlatGL2D _shaderFlat;
};

GL::Mesh generateLineMesh(Containers::ArrayView<const Vector2> lineSegments, bool loop = false) {
    return MeshTools::compileLines(Trade::MeshData{MeshPrimitive::Lines, {}, lineSegments, {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, lineSegments}
    }});
}

GL::Mesh generateLineMesh(std::initializer_list<Vector2> lineSegments, bool loop = false) {
    return generateLineMesh(Containers::arrayView(lineSegments), loop);
}

LineRenderingExample::LineRenderingExample(const Arguments& arguments): Platform::Application{arguments,
    Configuration{}
        .setTitle("Line Playground") // TODO change
        .setWindowFlags(Configuration::WindowFlag::Resizable) // TODO remove

} {
    /* A single line */
    _line = generateLineMesh({
        {0.0f, 0.0f}, {100.0f, 0.0f}
    });

    /* Joints at different angles */
    // TODO use an index buffer to actually mark the joins
    // TODO also demo various join styles
    Vector2 jointData[7*4];
    for(std::size_t i = 0; i != 5; ++i) {
        Vector2 center{i*75.0f, 0.0f};
        jointData[i*4 + 0] = center +
            Matrix3::rotation((i + 1)*-30.0_degf)
                .transformVector(Vector2::xAxis(-50.0f));
        jointData[i*4 + 1] = center;
        jointData[i*4 + 2] = center;
        jointData[i*4 + 3] = center + Vector2::xAxis(50.0f);
    }
    _joints = generateLineMesh(jointData);

    /* Circle, square, both are loops, converted as non-indexed line segments
       from a LineLoop primitive */
    _circle = MeshTools::compileLines(Primitives::circle2DWireframe(50));
    _square = MeshTools::compileLines(Primitives::squareWireframe());
    _cube = MeshTools::compileLines(MeshTools::compressIndices(Primitives::cubeWireframe(), MeshIndexType::UnsignedInt)); // TODO remove the compressIndices once compileLines() stops dying on 16bit indices

    /* Animation player set up. Repeating forever. */
    _timeline.start();
    _animationPlayer
        .add(_animationRotationTrack, _animationRotation)
        .add(_animationCubeRotationTrack, _animationCubeRotation)
        .add(_animationScalingTrack, _animationScaling)
        .add(_animationWidthTrack, _animationWidth)
        .setPlayCount(0)
        .play(_timeline.previousFrameTime());

    /* Shader defaults */
    for(auto shader: {&_shaderButtCaps,
                      &_shaderSquareCaps,
                      &_shaderRoundCaps,
                      &_shaderTriangleCaps})
    {
        (*shader)
            .setSmoothness(1.0f)
            .setViewportSize(Vector2{framebufferSize()});
    }
    for(auto shader: {&_shaderRoundCaps3D})
    {
        (*shader)
            .setSmoothness(1.0f)
            .setViewportSize(Vector2{framebufferSize()});
    }

    /* Blending for overlapping lines */
    // TODO won't be needed once discard is implemented for overlapping line
    //  joints
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::One,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

void LineRenderingExample::viewportEvent(ViewportEvent& event) {
    for(auto shader: {&_shaderSquareCaps,
                      &_shaderRoundCaps,
                      &_shaderTriangleCaps})
    {
        (*shader)
            .setViewportSize(Vector2{event.framebufferSize()});
    }

    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
}

void LineRenderingExample::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _animationPlayer.state() == Animation::State::Playing ?
            _animationPlayer.pause(_timeline.previousFrameTime()) :
            _animationPlayer.resume(_timeline.previousFrameTime());
    event.setAccepted();
}

void LineRenderingExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::F) {
        if(_circle.primitive() == GL::MeshPrimitive::Lines) {
            _circle.setPrimitive(MeshPrimitive::Triangles);
            _square.setPrimitive(MeshPrimitive::Triangles);
        } else {
            _circle.setPrimitive(MeshPrimitive::Lines);
            _square.setPrimitive(MeshPrimitive::Lines);
        }
    } else return;

    event.setAccepted();
    redraw();
}

void LineRenderingExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _timeline.nextFrame();
    _animationPlayer.advance(_timeline.previousFrameTime());

    /* Have a projection with origin bottom left for easier calculations */
    const Matrix3 projection =
        Matrix3::translation({-1.0f, -1.0f})*
        Matrix3::projection({640.0f, 480.0f}); // TODO er why here not in the class, or why not adapting to viewport event

    /* Draw a set of lines with different thickness */
    Float yPos = 25.0f;
    for(std::size_t i = 0; i != 8; ++i) {
        Float width = i + 1.0f;
            _shaderRoundCaps
            .setTransformationProjectionMatrix(projection*
                Matrix3::translation({25.0f, yPos + width/2.0f}))
            .setWidth(width)
            .draw(_line);
        yPos += width + 5.0f;
    }

    /* Draw a set of lines with different cap ends */
    for(auto i: {Containers::pair(0, &_shaderButtCaps),
                 Containers::pair(1, &_shaderSquareCaps),
                 Containers::pair(2, &_shaderRoundCaps),
                 Containers::pair(3, &_shaderTriangleCaps),}) {
        (*i.second())
            .setTransformationProjectionMatrix(projection*
                Matrix3::translation({515.0f, 30.0f + i.first()*20.0f}))
            .setWidth(20.0f)
            .draw(_line);
    }

    /* Draw joints at different angles */
    _shaderRoundCaps
        .setTransformationProjectionMatrix(projection*
            Matrix3::translation({155.0f, 150.0f}))
        .setWidth(20.0f)
        .draw(_joints);

    /* Draw the circle & square. Those are loops so it doesn't matter which
       cap style is used as there are no caps. */
    const Matrix3 circleTransformationProjection = projection*
        Matrix3::translation({120.0f, 350.0f})*
        Matrix3::scaling(Vector2{100.0f})*
        Matrix3::scaling(_animationScaling);
    const Matrix3 squareTransformationProjection = projection*
        Matrix3::translation({510.0f, 350.0f})*
        Matrix3::scaling(Vector2{100.0f/Constants::sqrt2()})*
        Matrix3::rotation(_animationRotation)*
        Matrix3::scaling(_animationScaling);
    if(_circle.primitive() == GL::MeshPrimitive::Triangles) {
        _shaderSquareCaps
            .setTransformationProjectionMatrix(circleTransformationProjection)
            .setWidth(_animationWidth)
            .draw(_circle);
        _shaderSquareCaps
            .setTransformationProjectionMatrix(squareTransformationProjection)
            .setWidth(_animationWidth)
            .draw(_square);
        _shaderRoundCaps3D
            .setTransformationProjectionMatrix(
                Matrix4::perspectiveProjection(75.0_degf, 4.0f/3.0f, 0.1f, 100.0f)*
                Matrix4::translation({0.0f, 2.75f, -10.0f})*
                Matrix4::rotationX(30.0_degf)*
                Matrix4::rotationY(_animationCubeRotation))
            .setWidth(_animationWidth)
            .setSmoothness(1.0f)
            .draw(_cube);
    } else {
        _shaderFlat
            .setTransformationProjectionMatrix(circleTransformationProjection)
            .draw(_circle);
        _shaderFlat
            .setTransformationProjectionMatrix(squareTransformationProjection)
            .draw(_square);
    }

    redraw();
    swapBuffers();
}

}

MAGNUM_APPLICATION_MAIN(LineRenderingExample)
