/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
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

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Pointer.h>
#include <Corrade/Utility/Arguments.h>
#include <Magnum/DebugTools/FrameProfiler.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Icosphere.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData.h>

#include "LooseOctree.h"
#include "../arcball/ArcBall.h"

namespace Magnum { namespace Examples {

struct SphereInstanceData {
    Matrix4 transformationMatrix;
    Matrix3x3 normalMatrix;
    Color3 color;
};

struct BoxInstanceData {
    Matrix4 transformationMatrix;
    Color3 color;
};

class OctreeExample: public Platform::Application {
    public:
        explicit OctreeExample(const Arguments& arguments);

    protected:
        void viewportEvent(ViewportEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;

        void movePoints();
        void collisionDetectionAndHandlingBruteForce();
        void collisionDetectionAndHandlingUsingOctree();
        void checkCollisionWithSubTree(const OctreeNode& node, std::size_t i,
            const Vector3& ppos, const Vector3& pvel, const Range3D& bounds);
        void drawSpheres();
        void drawTreeNodeBoundingBoxes();

        Containers::Optional<ArcBall> _arcballCamera;
        Matrix4 _projectionMatrix;

        /* Points data as spheres with size */
        Containers::Array<Vector3> _spherePositions;
        Containers::Array<Vector3> _sphereVelocities;
        Float _sphereRadius, _sphereVelocity;
        bool _animation = true;
        bool _collisionDetectionByOctree = true;

        /* Octree and boundary boxes */
        Containers::Pointer<LooseOctree> _octree;

        /* Profiling */
        DebugTools::GLFrameProfiler _profiler{
            DebugTools::GLFrameProfiler::Value::FrameTime|
            DebugTools::GLFrameProfiler::Value::CpuDuration, 180};

        /* Spheres rendering */
        GL::Mesh _sphereMesh{NoCreate};
        GL::Buffer _sphereInstanceBuffer{NoCreate};
        Shaders::Phong _sphereShader{NoCreate};
        Containers::Array<SphereInstanceData> _sphereInstanceData;

        /* Treenode bounding boxes rendering */
        GL::Mesh _boxMesh{NoCreate};
        GL::Buffer _boxInstanceBuffer{NoCreate};
        Shaders::Flat3D _boxShader{NoCreate};
        Containers::Array<BoxInstanceData> _boxInstanceData;
        bool _drawBoundingBoxes = true;
};

using namespace Math::Literals;

OctreeExample::OctreeExample(const Arguments& arguments) : Platform::Application{arguments, NoCreate} {
    Utility::Arguments args;
    args.addOption('s', "spheres", "2000")
            .setHelp("spheres", "number of spheres to simulate", "N")
        .addOption('r', "sphere-radius", "0.0333")
            .setHelp("sphere-radius", "sphere radius", "R")
        .addOption('v', "sphere-velocity", "0.05")
            .setHelp("sphere-velocity", "sphere velocity", "V")
        .addSkippedPrefix("magnum")
        .parse(arguments.argc, arguments.argv);

    _sphereRadius = args.value<Float>("sphere-radius");
    _sphereVelocity = args.value<Float>("sphere-velocity");

    /* Setup window and parameters */
    {
        const Vector2 dpiScaling = this->dpiScaling({});
        Configuration conf;
        conf.setTitle("Magnum Octree Example")
            .setSize(conf.size(), dpiScaling)
            .setWindowFlags(Configuration::WindowFlag::Resizable);
        GLConfiguration glConf;
        glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);
        if(!tryCreate(conf, glConf)) {
            create(conf, glConf.setSampleCount(0));
        }

        GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
        GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

        /* Loop at 60 Hz max */
        setSwapInterval(1);
        setMinimalLoopPeriod(16);
    }

    /* Setup camera */
    {
        const Vector3 eye = Vector3::zAxis(5.0f);
        const Vector3 viewCenter;
        const Vector3 up = Vector3::yAxis();
        const Deg fov = 45.0_degf;
        _arcballCamera.emplace(eye, viewCenter, up, fov, windowSize());
        _arcballCamera->setLagging(0.85f);

        _projectionMatrix = Matrix4::perspectiveProjection(fov,
            Vector2{framebufferSize()}.aspectRatio(), 0.01f, 100.0f);
    }

    /* Setup points (render as spheres) */
    {
        const UnsignedInt numSpheres = args.value<UnsignedInt>("spheres");
        _spherePositions = Containers::Array<Vector3>{Containers::NoInit,
            numSpheres};
        _sphereVelocities = Containers::Array<Vector3>{Containers::NoInit,
            numSpheres};
        _sphereInstanceData = Containers::Array<SphereInstanceData>{
            Containers::NoInit, numSpheres};

        for(std::size_t i = 0; i < numSpheres; ++i) {
            const Vector3 tmpPos = Vector3(std::rand(), std::rand(), std::rand())/
                Float(RAND_MAX);
            const Vector3 tmpVel = Vector3(std::rand(), std::rand(), std::rand())/
                Float(RAND_MAX);
            _spherePositions[i] = tmpPos*2.0f - Vector3{1.0f};
            _spherePositions[i].y() *= 0.5f;
            _sphereVelocities[i] = (tmpVel*2.0f - Vector3{1.0f}).resized(_sphereVelocity);

            /* Fill in the instance data. Most of this stays the same, except
               for the translation */
            _sphereInstanceData[i].transformationMatrix =
                Matrix4::translation(_spherePositions[i])*
                Matrix4::scaling(Vector3{_sphereRadius});
            _sphereInstanceData[i].normalMatrix =
                _sphereInstanceData[i].transformationMatrix.normalMatrix();
            _sphereInstanceData[i].color = Color3{tmpPos};
        }

        _sphereShader = Shaders::Phong{
            Shaders::Phong::Flag::VertexColor|
            Shaders::Phong::Flag::InstancedTransformation};
        _sphereInstanceBuffer = GL::Buffer{};
        _sphereMesh = MeshTools::compile(Primitives::icosphereSolid(2));
        _sphereMesh.addVertexBufferInstanced(_sphereInstanceBuffer, 1, 0,
            Shaders::Phong::TransformationMatrix{},
            Shaders::Phong::NormalMatrix{},
            Shaders::Phong::Color3{});
        _sphereMesh.setInstanceCount(_sphereInstanceData.size());
    }

    /* Setup octree */
    {
        /* Octree nodes should have half width no smaller than the sphere
           radius */
        _octree.emplace(Vector3{0}, 1.0f, Math::max(_sphereRadius, 0.1f));

        _octree->setPoints(_spherePositions);
        _octree->build();
        Debug{} << "  Allocated nodes:" << _octree->numAllocatedNodes();
        Debug{} << "  Max number of points per node:" << _octree->maxNumPointInNodes();

        /* Disable profiler by default */
        _profiler.disable();
    }

    /* Treenode bounding boxes render variables */
    {
        _boxShader = Shaders::Flat3D{
            Shaders::Flat3D::Flag::VertexColor|
            Shaders::Flat3D::Flag::InstancedTransformation};
        _boxInstanceBuffer = GL::Buffer{};
        _boxMesh = MeshTools::compile(Primitives::cubeWireframe());
        _boxMesh.addVertexBufferInstanced(_boxInstanceBuffer, 1, 0,
            Shaders::Flat3D::TransformationMatrix{},
            Shaders::Flat3D::Color3{});
    }

    Debug{} << "Collision detection using octree";
}

void OctreeExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);
    _profiler.beginFrame();

    if(_animation) {
        if(_collisionDetectionByOctree)
            collisionDetectionAndHandlingUsingOctree();
        else
            collisionDetectionAndHandlingBruteForce();

        movePoints();

        if(_collisionDetectionByOctree)
            _octree->update();
    }

    /* Update camera before drawing instances */
    const bool moving = _arcballCamera->updateTransformation();

    drawSpheres();
    drawTreeNodeBoundingBoxes();

    _profiler.endFrame();
    _profiler.printStatistics(10);

    swapBuffers();

    /* If the camera is moving or the animation is running, redraw immediately */
    if(moving || _animation) redraw();
}

void OctreeExample::collisionDetectionAndHandlingBruteForce() {
    for(std::size_t i = 0; i < _spherePositions.size(); ++i) {
        const Vector3 ppos = _spherePositions[i];
        const Vector3 pvel = _sphereVelocities[i];
        for(std::size_t j = i + 1; j < _spherePositions.size(); ++j) {
            const Vector3 qpos = _spherePositions[j];
            const Vector3 qvel = _sphereVelocities[j];
            const Vector3 velpq = pvel - qvel;
            const Vector3 pospq = ppos - qpos;
            const Float vp = Math::dot(velpq, pospq);
            if(vp < 0.0f) {
                const Float dpq = pospq.length();
                if(dpq < 2.0f*_sphereRadius) {
                    const Vector3 vNormal = vp * pospq / (dpq * dpq);
                    _sphereVelocities[i] = (_sphereVelocities[i] - vNormal).resized(_sphereVelocity);
                    _sphereVelocities[j] = (_sphereVelocities[j] + vNormal).resized(_sphereVelocity);
                }
            }
        }
    }
}

void OctreeExample::collisionDetectionAndHandlingUsingOctree() {
    const OctreeNode& rootNode = _octree->rootNode();
    for(std::size_t i = 0; i < _spherePositions.size(); ++i) {
        checkCollisionWithSubTree(rootNode, i,
            _spherePositions[i], _sphereVelocities[i],
            Range3D::fromCenter(_spherePositions[i], Vector3{_sphereRadius}));
    }
}

void OctreeExample::checkCollisionWithSubTree(const OctreeNode& node,
    std::size_t i, const Vector3& ppos, const Vector3& pvel, const Range3D& bounds)
{
    if(!node.looselyOverlaps(bounds)) return;

    if(!node.isLeaf()) {
        for(std::size_t childIdx = 0; childIdx < 8; ++childIdx) {
            const OctreeNode& child = node.childNode(childIdx);
            checkCollisionWithSubTree(child, i, ppos, pvel, bounds);
        }
    }

    for(const OctreePoint* const point: node.pointList()) {
        const std::size_t j = point->idx();
        if(j > i) {
            const Vector3 qpos = _spherePositions[j];
            const Vector3 qvel = _sphereVelocities[j];
            const Vector3 velpq = pvel - qvel;
            const Vector3 pospq = ppos - qpos;
            const Float vp = Math::dot(velpq, pospq);
            if(vp < 0.0f) {
                const Float dpq = pospq.length();
                if(dpq < 2.0f*_sphereRadius) {
                    const Vector3 vNormal = vp*pospq/(dpq*dpq);
                    _sphereVelocities[i] = (_sphereVelocities[i] - vNormal).resized(_sphereVelocity);
                    _sphereVelocities[j] = (_sphereVelocities[j] + vNormal).resized(_sphereVelocity);
                }
            }
        }
    }
}

void OctreeExample::movePoints() {
    constexpr Float dt = 1.0f/120.0f;

    for(std::size_t i = 0; i < _spherePositions.size(); ++i) {
        Vector3 pos = _spherePositions[i] + _sphereVelocities[i] * dt;
        for(std::size_t j = 0; j < 3; ++j) {
            if(pos[j] < -1.0f || pos[j] > 1.0f)
                _sphereVelocities[i][j] = -_sphereVelocities[i][j];
            pos[j] = Math::clamp(pos[j], -1.0f, 1.0f);
        }

        _spherePositions[i] = pos;
    }
}

void OctreeExample::drawSpheres() {
    for(std::size_t i = 0; i != _spherePositions.size(); ++i)
        _sphereInstanceData[i].transformationMatrix.translation() =
            _spherePositions[i];

    _sphereInstanceBuffer.setData(_sphereInstanceData, GL::BufferUsage::DynamicDraw);
    _sphereShader
        .setProjectionMatrix(_projectionMatrix)
        .setTransformationMatrix(_arcballCamera->viewMatrix())
        .setNormalMatrix(_arcballCamera->viewMatrix().normalMatrix())
        .draw(_sphereMesh);
}

void OctreeExample::drawTreeNodeBoundingBoxes() {
    arrayResize(_boxInstanceData, 0);

    /* Always draw the root node */
    arrayAppend(_boxInstanceData, Containers::InPlaceInit,
        _arcballCamera->viewMatrix()*
        Matrix4::translation(_octree->center())*
        Matrix4::scaling(Vector3{_octree->halfWidth()}), 0x00ffff_rgbf);

    /* Draw the remaining non-empty nodes */
    if(_drawBoundingBoxes) {
        const auto& activeTreeNodeBlocks = _octree->activeTreeNodeBlocks();
        for(OctreeNodeBlock* const pNodeBlock : activeTreeNodeBlocks) {
            for(std::size_t childIdx = 0; childIdx < 8; ++childIdx) {
                const OctreeNode& pNode = pNodeBlock->_nodes[childIdx];

                /* Non-empty node */
                if(!pNode.isLeaf() || pNode.pointCount() > 0) {
                    const Matrix4 t = _arcballCamera->viewMatrix() *
                        Matrix4::translation(pNode.center())*
                        Matrix4::scaling(Vector3{pNode.halfWidth()});
                    arrayAppend(_boxInstanceData, Containers::InPlaceInit, t,
                        0x197f99_rgbf);
                }
            }
        }
    }

    _boxInstanceBuffer.setData(_boxInstanceData, GL::BufferUsage::DynamicDraw);
    _boxMesh.setInstanceCount(_boxInstanceData.size());
    _boxShader.setTransformationProjectionMatrix(_projectionMatrix)
        .draw(_boxMesh);
}

void OctreeExample::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
    _arcballCamera->reshape(event.windowSize());

    _projectionMatrix = Matrix4::perspectiveProjection(_arcballCamera->fov(),
        Vector2{event.framebufferSize()}.aspectRatio(), 0.01f, 100.0f);
}

void OctreeExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::B) {
        _drawBoundingBoxes ^= true;

    } else if(event.key() == KeyEvent::Key::O) {
        if((_collisionDetectionByOctree ^= true))
            Debug{} << "Collision detection using octree";
        else
            Debug{} << "Collision detection using brute force";
        /* Reset the profiler to avoid measurements of the two methods mixed
           together */
        if(_profiler.isEnabled()) _profiler.enable();

    } else if(event.key() == KeyEvent::Key::P) {
        if(_profiler.isEnabled()) _profiler.disable();
        else _profiler.enable();

    } else if(event.key() == KeyEvent::Key::R) {
        _arcballCamera->reset();

    } else if(event.key() == KeyEvent::Key::Space) {
        _animation ^= true;

    } else return;

    event.setAccepted();
    redraw();
}

void OctreeExample::mousePressEvent(MouseEvent& event) {
    /* Enable mouse capture so the mouse can drag outside of the window */
    /** @todo replace once https://github.com/mosra/magnum/pull/419 is in */
    SDL_CaptureMouse(SDL_TRUE);
    _arcballCamera->initTransformation(event.position());
    event.setAccepted();
    redraw(); /* camera has changed, redraw! */
}

void OctreeExample::mouseReleaseEvent(MouseEvent&) {
    /* Disable mouse capture again */
    /** @todo replace once https://github.com/mosra/magnum/pull/419 is in */
    SDL_CaptureMouse(SDL_FALSE);
}

void OctreeExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!event.buttons()) return;

    if(event.modifiers() & MouseMoveEvent::Modifier::Shift)
        _arcballCamera->translate(event.position());
    else _arcballCamera->rotate(event.position());

    event.setAccepted();
    redraw(); /* camera has changed, redraw! */
}

void OctreeExample::mouseScrollEvent(MouseScrollEvent& event) {
    const Float delta = event.offset().y();
    if(Math::abs(delta) < 1.0e-2f) return;

    _arcballCamera->zoom(delta);

    event.setAccepted();
    redraw(); /* camera has changed, redraw! */
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::OctreeExample)
