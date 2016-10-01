/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016 —
            Vladimír Vondruš <mosra@centrum.cz>
        2016 — Bill Robinson <airbaggins@gmail.com>

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
#include <Magnum/Texture.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Renderer.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Capsule.h>
#include <Magnum/Primitives/Plane.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/AbstractObject.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include "ShadowCasterShader.h"
#include "ShadowReceiverShader.h"
#include "ShadowLight.h"
#include "ShadowCasterDrawable.h"
#include "ShadowReceiverDrawable.h"
#include "DebugLines.h"

namespace Magnum { namespace Examples {

const float MAIN_CAMERA_NEAR = 0.01f;
const float MAIN_CAMERA_FAR = 100.0f;

using namespace Magnum::Math::Literals;

typedef Magnum::SceneGraph::Object<Magnum::SceneGraph::MatrixTransformation3D> Object3D;
typedef Magnum::SceneGraph::Scene<Magnum::SceneGraph::MatrixTransformation3D> Scene3D;

class ShadowsExample: public Platform::Application {
public:
    explicit ShadowsExample(const Arguments& arguments);

private:
    struct Model {
        Buffer indexBuffer, vertexBuffer;
        Mesh mesh;
        float radius;
    };

    void drawEvent() override;
    void mousePressEvent(MouseEvent& event) override;
    void mouseReleaseEvent(MouseEvent& event) override;
    void mouseMoveEvent(MouseMoveEvent& event) override;
    void keyPressEvent(KeyEvent &event) override;
    void keyReleaseEvent(KeyEvent &event) override;

    void addModel(const Trade::MeshData3D &meshData3D);
    void renderDebugLines();
    Object3D* createSceneObject(Model &model, bool makeCaster, bool makeReceiver);
    void recompileReceiverShader(size_t numLayers);
    void setShadowMapSize(Vector2i shadowMapSize);
    void setShadowSplitExponent(float power);

    Scene3D scene;
    Magnum::SceneGraph::DrawableGroup3D shadowCasterDrawables;
    Magnum::SceneGraph::DrawableGroup3D shadowReceiverDrawables;
    ShadowCasterShader shadowCasterShader;
    std::unique_ptr<ShadowReceiverShader> shadowReceiverShader;

    DebugLines debugLines;

    Object3D shadowLightObject;
    ShadowLight shadowLight;
    Object3D mainCameraObject;
    Magnum::SceneGraph::Camera3D mainCamera;
    Object3D debugCameraObject;
    Magnum::SceneGraph::Camera3D debugCamera;

    Object3D* activeCameraObject;
    Magnum::SceneGraph::Camera3D* activeCamera;

    std::vector<Model> models;

    Magnum::Vector3 mainCameraVelocity;

    float shadowBias;
    float layerSplitExponent;
    Magnum::Vector2i shadowMapSize;
    int shadowMapFaceCullMode;
    bool shadowStaticAlignment;
};

ShadowsExample::ShadowsExample(const Arguments& arguments): Platform::Application{arguments, Configuration{}.setTitle("Magnum Shadows Example")}
,   scene()
,   shadowLightObject(&scene)
,   shadowLight(shadowLightObject)
,   mainCameraObject(&scene)
,   mainCamera(mainCameraObject)
,   debugCameraObject(&scene)
,   debugCamera(debugCameraObject)
,   shadowBias(0.003f)
,   layerSplitExponent(3.0f)
,   shadowMapSize(1024,1024)
,   shadowMapFaceCullMode(1)
{
    shadowLight.setupShadowmaps(3, shadowMapSize);
    shadowReceiverShader.reset(new ShadowReceiverShader(shadowLight.getNumLayers()));
    shadowReceiverShader->setShadowBias(shadowBias);

    Renderer::enable(Renderer::Feature::DepthTest);
    Renderer::enable(Renderer::Feature::FaceCulling);

    addModel(Primitives::Cube::solid());
    addModel(Primitives::Capsule3D::solid(1,1,4,1));
    addModel(Primitives::Capsule3D::solid(6,1,9,1));

    auto ground = createSceneObject(models[0], false, true);
    ground->setTransformation(Magnum::Matrix4::scaling({100,1,100}));

    for (int i = 0; i < 200; i++) {
        auto& model = models[rand() % models.size()];
        auto object = createSceneObject(model, true, true);
        object->setTransformation(Magnum::Matrix4::translation({rand()*100.0f/RAND_MAX - 50, rand()*5.0f/RAND_MAX, rand()*100.0f/RAND_MAX - 50}));
    }

    shadowLight.setupSplitDistances(MAIN_CAMERA_NEAR, MAIN_CAMERA_FAR, layerSplitExponent);

    mainCamera.setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, Vector2{defaultFramebuffer.viewport().size()}.aspectRatio(), MAIN_CAMERA_NEAR, MAIN_CAMERA_FAR));
    mainCameraObject.setTransformation(Matrix4::translation({0,3,0}));

    debugCamera.setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, Vector2{defaultFramebuffer.viewport().size()}.aspectRatio(), MAIN_CAMERA_NEAR / 4.0f, MAIN_CAMERA_FAR * 4.0f));
    debugCameraObject.setTransformation(Matrix4::lookAt({100,50,0},{0,0,-30},{0,1,0}));

    activeCamera = &mainCamera;
    activeCameraObject = &mainCameraObject;

    shadowLightObject.setTransformation(Matrix4::lookAt({3.0f,1,2.0f}, {0,0,0}, {0,1,0}));
}

Object3D* ShadowsExample::createSceneObject(ShadowsExample::Model &model, bool makeCaster, bool makeReceiver) {
    auto object = new Object3D(&scene);

    if (makeCaster) {
        auto caster = new ShadowCasterDrawable(*object, &shadowCasterDrawables);
        caster->setShader(&shadowCasterShader);
        caster->setMesh(&model.mesh, model.radius);
    }

    if (makeReceiver) {
        auto receiver = new ShadowReceiverDrawable(*object, &shadowReceiverDrawables);
        receiver->setShader(shadowReceiverShader.get());
        receiver->setMesh(&model.mesh);
    }
    return object;
}

void ShadowsExample::addModel(const Trade::MeshData3D &meshData3D) {
    models.emplace_back();
    auto& model = this->models.back();

    model.vertexBuffer.setData(MeshTools::interleave(meshData3D.positions(0), meshData3D.normals(0)), BufferUsage::StaticDraw);

    float maxMagnitudeSquared = 0.0f;
    for (auto position : meshData3D.positions(0)) {
        auto magnitudeSquared = Math::dot(position,position);
        if (magnitudeSquared > maxMagnitudeSquared) {
            maxMagnitudeSquared = magnitudeSquared;
        }
    }
    model.radius = std::sqrt(maxMagnitudeSquared);

    Containers::Array<char> indexData;
    Mesh::IndexType indexType;
    UnsignedInt indexStart, indexEnd;
    std::tie(indexData, indexType, indexStart, indexEnd) = MeshTools::compressIndices(meshData3D.indices());
    model.indexBuffer.setData(indexData, BufferUsage::StaticDraw);

    model.mesh.setPrimitive(meshData3D.primitive())
        .setCount(meshData3D.indices().size())
        .addVertexBuffer(model.vertexBuffer, 0, Shaders::Phong::Position{}, Shaders::Phong::Normal{})
        .setIndexBuffer(model.indexBuffer, 0, indexType, indexStart, indexEnd);

}

void ShadowsExample::drawEvent() {

    if (!mainCameraVelocity.isZero()) {
        auto transform = activeCameraObject->transformation();
        transform.translation() += transform.rotation() * mainCameraVelocity * 0.3f;
        activeCameraObject->setTransformation(transform);
        redraw();
    }

    auto screenDirection = shadowStaticAlignment ? Vector3{0,0,1} : mainCameraObject.transformation()[2].xyz();
    /* You only really need to do this when your camera moves */
    shadowLight.setTarget({3, 2, 3}, screenDirection, mainCamera);

    /* You can use face culling, depending on your geometry. You might want to render only back faces for shadows. */
    switch (shadowMapFaceCullMode) {
        case 0:
            Magnum::Renderer::disable(Magnum::Renderer::Feature::FaceCulling);
            break;
        case 2:
            Magnum::Renderer::setFaceCullingMode(Magnum::Renderer::PolygonFacing::Front);
            break;
    }

    /* Create the shadow map textures. */
    shadowLight.render(shadowCasterDrawables);

    switch (shadowMapFaceCullMode) {
        case 0:
            Magnum::Renderer::enable(Magnum::Renderer::Feature::FaceCulling);
            break;
        case 2:
            Magnum::Renderer::setFaceCullingMode(Magnum::Renderer::PolygonFacing::Back);
            break;
    }

    Magnum::Renderer::setClearColor({0.1f,0.1f,0.4f,1.0f});
    defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);

    Corrade::Containers::Array<Magnum::Matrix4> shadowMatrices(Corrade::Containers::NoInit, shadowLight.getNumLayers());
    for (auto layerIndex = 0u; layerIndex < shadowLight.getNumLayers(); layerIndex++) {
        shadowMatrices[layerIndex] = shadowLight.getLayerMatrix(layerIndex);
    }

    shadowReceiverShader->setShadowmapMatrices(shadowMatrices);
    shadowReceiverShader->setShadowmapTexture(*shadowLight.getShadowTexture());
    shadowReceiverShader->setLightDirection(shadowLightObject.transformation()[2].xyz());

    activeCamera->draw(shadowReceiverDrawables);

    renderDebugLines();

    swapBuffers();
}

void ShadowsExample::renderDebugLines() {
    if (activeCamera == &debugCamera) {
        Matrix4 unbiasMatrix{
                {2,  0,  0,  0},
                {0,  2,  0,  0},
                {0,  0,  2,  0},
                {-1, -1, -1, 1}};
        debugLines.reset();
        auto imvp = (mainCamera.projectionMatrix() * mainCamera.cameraMatrix()).inverted();
        for (auto layerIndex = 0u; layerIndex < shadowLight.getNumLayers(); layerIndex++) {
            auto layerMatrix = shadowLight.getLayerMatrix(layerIndex);
            auto hue = layerIndex * 360.0_degf / shadowLight.getNumLayers();
            debugLines.addFrustum((unbiasMatrix * layerMatrix).inverted(),
                                  Color3::fromHSV(hue, 1.0f, 0.5f));
            debugLines.addFrustum(imvp,
                                  Color3::fromHSV(hue, 1.0f, 1.0f),
                                  layerIndex == 0 ? 0 : shadowLight.getCutZ(layerIndex - 1),
                                  shadowLight.getCutZ(layerIndex));
        }
        debugLines.draw(activeCamera->projectionMatrix() * activeCamera->cameraMatrix());
    }
}

void ShadowsExample::mousePressEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left) return;

    event.setAccepted();
}

void ShadowsExample::mouseReleaseEvent(MouseEvent& event) {

    event.setAccepted();
    redraw();
}

void ShadowsExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    auto transform = activeCameraObject->transformation();

    constexpr float angleScale = 0.01f;
    auto angleX = event.relativePosition().x() * angleScale;
    auto angleY = event.relativePosition().y() * angleScale;
    if (angleX != 0 || angleY != 0) {
        transform = Matrix4::lookAt(transform.translation(), transform.translation() - transform.rotationScaling() * Magnum::Vector3{-angleX, angleY, 1}, {0,1,0});

        activeCameraObject->setTransformation(transform);
    }

    event.setAccepted();
    redraw();
}

void ShadowsExample::keyPressEvent(Platform::Sdl2Application::KeyEvent &event) {
    if (event.key() == KeyEvent::Key::Up) {
        mainCameraVelocity.z() = -1;
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::Down) {
        mainCameraVelocity.z() = 1;
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::PageUp) {
        mainCameraVelocity.y() = 1;
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::PageDown) {
        mainCameraVelocity.y() = -1;
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::Right) {
        mainCameraVelocity.x() = 1;
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::Left) {
        mainCameraVelocity.x() = -1;
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::F1) {
        activeCamera = &mainCamera;
        activeCameraObject = &mainCameraObject;
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::F2) {
        activeCamera = &debugCamera;
        activeCameraObject = &debugCameraObject;
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::F3) {
        shadowMapFaceCullMode = (shadowMapFaceCullMode + 1) % 3;
        Debug() << "Face cull mode: " << (shadowMapFaceCullMode == 0 ? "no cull" : shadowMapFaceCullMode == 1 ? "cull back" : "cull front");
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::F4) {
        shadowStaticAlignment = !shadowStaticAlignment;
        Debug() << "Shadow alignment: " << (shadowStaticAlignment ? "static" : "camera direction");
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::One) {
        setShadowSplitExponent(layerSplitExponent *= 1.125f);
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::Two) {
        setShadowSplitExponent(layerSplitExponent /= 1.125f);
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::Three) {
        shadowReceiverShader->setShadowBias(shadowBias /= 1.125f);
        Debug() << "Shadow bias " << shadowBias;
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::Four) {
        shadowReceiverShader->setShadowBias(shadowBias *= 1.125f);
        Debug() << "Shadow bias " << shadowBias;
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::Five) {
        auto numLayers = shadowLight.getNumLayers() - 1;
        if (numLayers >= 1) {
            shadowLight.setupShadowmaps(numLayers, shadowMapSize);
            recompileReceiverShader(numLayers);
            shadowLight.setupSplitDistances(MAIN_CAMERA_NEAR, MAIN_CAMERA_FAR, layerSplitExponent);
            Debug() << "Shadow map size " << shadowMapSize << " x " << shadowLight.getNumLayers() << " layers";
            event.setAccepted();
        }
    }
    else if (event.key() == KeyEvent::Key::Six) {
        auto numLayers = shadowLight.getNumLayers() + 1;
        if (numLayers <= 32) {
            shadowLight.setupShadowmaps(numLayers, shadowMapSize);
            recompileReceiverShader(numLayers);
            shadowLight.setupSplitDistances(MAIN_CAMERA_NEAR, MAIN_CAMERA_FAR, layerSplitExponent);
            Debug() << "Shadow map size " << shadowMapSize << " x " << shadowLight.getNumLayers() << " layers";
            event.setAccepted();
        }
    }
    else if (event.key() == KeyEvent::Key::Seven) {
        setShadowMapSize(shadowMapSize / 2);
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::Eight) {
        setShadowMapSize(shadowMapSize * 2);
        event.setAccepted();
    }
    redraw();
}

void ShadowsExample::setShadowSplitExponent(float power) {
    shadowLight.setupSplitDistances(MAIN_CAMERA_NEAR, MAIN_CAMERA_FAR, power);
    std::string buf = "";
    for (size_t layer = 0; layer < shadowLight.getNumLayers(); layer++) {
        if (layer) buf += ", ";
        buf += std::to_string(shadowLight.getCutDistance(MAIN_CAMERA_NEAR, MAIN_CAMERA_FAR, layer));
    }
    Debug() << "Shadow splits power=" << power << " cut points: " << buf;
}

void ShadowsExample::setShadowMapSize(Vector2i shadowMapSize) {
    if (shadowMapSize.x() <= Magnum::Texture2D::maxSize().x() && shadowMapSize.y() <= Magnum::Texture2D::maxSize().y() &&
        shadowMapSize.x() >= 1 && shadowMapSize.y() >= 1) {
        this->shadowMapSize = shadowMapSize;
        shadowLight.setupShadowmaps(shadowLight.getNumLayers(), shadowMapSize);
        Debug() << "Shadow map size " << shadowMapSize << " x " << shadowLight.getNumLayers() << " layers";
    }
}

void ShadowsExample::recompileReceiverShader(size_t numLayers) {
    shadowReceiverShader.reset(new ShadowReceiverShader(numLayers));
    shadowReceiverShader->setShadowBias(shadowBias);
    for(size_t i = 0; i < shadowReceiverDrawables.size(); i++) {
        auto& drawable = static_cast<ShadowReceiverDrawable&>(shadowReceiverDrawables[i]);
        drawable.setShader(shadowReceiverShader.get());
    }
}

void ShadowsExample::keyReleaseEvent(Platform::Sdl2Application::KeyEvent &event) {
    if (event.key() == KeyEvent::Key::Up || event.key() == KeyEvent::Key::Down) {
        mainCameraVelocity.z() = 0;
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::PageDown || event.key() == KeyEvent::Key::PageUp) {
        mainCameraVelocity.y() = 0;
        event.setAccepted();
    }
    else if (event.key() == KeyEvent::Key::Right || event.key() == KeyEvent::Key::Left) {
        mainCameraVelocity.x() = 0;
        event.setAccepted();
    }
    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::ShadowsExample)
