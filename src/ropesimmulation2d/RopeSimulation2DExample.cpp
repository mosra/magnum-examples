/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022 — Vladimír Vondruš <mosra@centrum.cz>
        2023 — Michal Mikula <miso.mikula@gmail.com>

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

#include <set>
#include <queue>
#include <Corrade/Containers/Pointer.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/Containers/EnumSet.h>
#include <Corrade/Utility/StlMath.h>
#include <Corrade/Containers/Optional.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/PixelFormat.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/FunctionsBatch.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/SceneGraph/TranslationRotationScalingTransformation2D.h>
#include <Magnum/Timeline.h>
#include <Magnum/Trade/MeshData.h>

namespace Magnum { namespace Examples {

using namespace Magnum::Math::Literals;

namespace Utils {
    int orientation(const Vector2& p, const Vector2& q, const Vector2& r) {
        Float val = (q.y() - p.y()) * (r.x() - q.x()) - (q.x() - p.x()) * (r.y() - q.y());
        if (fabs(val) < 1e-9)
            return 0;
        return (val > 0) ? 1 : 2;
    }

    bool isWithinSegment(const Vector2& p, const Vector2& q, const Vector2& r) {
        if (q.x() <= fmax(p.x(), r.x()) && q.x() >= fmin(p.x(), r.x()) && q.y() <= fmax(p.y(), r.y()) && q.y() >= fmin(p.y(), r.y()))
            return true;
        return false;
    }

    bool doLineSegmentsIntersect(const Vector2& p1, const Vector2& q1, const Vector2& p2, const Vector2& q2) {
        int o1 = orientation(p1, q1, p2);
        int o2 = orientation(p1, q1, q2);
        int o3 = orientation(p2, q2, p1);
        int o4 = orientation(p2, q2, q1);

        if (o1 != o2 && o3 != o4) return true;

        if (o1 == 0 && isWithinSegment(p1, p2, q1)) return true;
        if (o2 == 0 && isWithinSegment(p1, q2, q1)) return true;
        if (o3 == 0 && isWithinSegment(p2, p1, q2)) return true;
        if (o4 == 0 && isWithinSegment(p2, q1, q2)) return true;

        return false;
    }
}

namespace {
    static Float RopeNodeDistance = 0.5f;
    static Float RopeSimulationDelta = 0.01f;
    static int32_t RopeConstraintIterations = 2;
    static Vector2 RopeGravity{ 0.0f, -9.89f };
}

struct Rope {
    struct RopeNode;

    /* Create "rectangular rope". */
    Rope(Vector2 rectMin, Vector2 rectMax) {
        _pointsX = (rectMax.x() - rectMin.x()) / RopeNodeDistance;
        _pointsY = (rectMax.y() - rectMin.y()) / RopeNodeDistance;

        /* Create nodes. */
        for (size_t x = 0; x < _pointsX; x++) {
            for (size_t y = 0; y < _pointsY; y++) {
                _nodes.push_back(Vector2{ rectMin.x() + x * RopeNodeDistance, rectMin.y() + y * RopeNodeDistance });
            }
        }

        /* Create hierarchy. */
        for (size_t x = 0; x < _pointsX; x++) {
            for (size_t y = 0; y < _pointsY; y++) {
                if (x > 0)
                    _nodes[x * _pointsY + y].childs.push_back(_nodes[(x - 1) * _pointsY + y]);
                if (x < _pointsX - 1)
                    _nodes[x * _pointsY + y].childs.push_back(_nodes[(x + 1) * _pointsY + y]);
                if (y > 0)
                    _nodes[x * _pointsY + y].childs.push_back(_nodes[x * _pointsY + y - 1]);
                if (y < _pointsY - 1)
                    _nodes[x * _pointsY + y].childs.push_back(_nodes[x * _pointsY + y + 1]);
            }
        }

        /* Rope with mass 0 does not move. */
        for (size_t x = 0; x < _pointsX; x++)
            _nodes[x * _pointsY + _pointsY - 1].mass = 0.0f;
    }

    std::vector<Vector2> getLinePoints() {
        std::vector<Vector2> result(_nodes.size());
        for (size_t i = 0; i < _nodes.size(); i++)
            result[i] = _nodes[i].position;
        return result;
    }

    /* Return vector of pairs for each line. */
    std::vector<Vector2> getLinePairs() {
        std::set<const RopeNode*> visited;
        std::vector<Vector2> linePoints;
        std::queue< const RopeNode*> nodes;

        for (const RopeNode& parent : _nodes) {
            if (visited.count(&parent))
                continue;
            nodes.push(&parent);

            while (!nodes.empty()) {
                const RopeNode* node = nodes.front();
                nodes.pop();

                if (visited.count(node))
                    continue;
                visited.insert(node);

                for (const RopeNode& child : node->childs) {
                    linePoints.push_back(node->position);
                    linePoints.push_back(child.position);

                    nodes.push(&child);
                }
            }
        }

        return linePoints;
    }

    void simulate(Corrade::Containers::Optional<Vector2> accelerateToward) {
        simulateStep(accelerateToward);
        for (int32_t i = 0; i < RopeConstraintIterations; i++)
            applyConstraints();
    }

    void simulateStep(Corrade::Containers::Optional<Vector2> accelerateToward) {
        for (auto& node : _nodes) {
            if (node.mass == 0.0f)
                continue;

            auto move = node.position - node.positionOld;
            Vector2 acceleration = RopeGravity;

            if (accelerateToward)
                acceleration = (*accelerateToward - node.position);

            node.positionOld = node.position;
            node.position += move + RopeSimulationDelta * RopeSimulationDelta * acceleration;
        }
    }

    void applyConstraints() {
        for (RopeNode& node1 : _nodes) {
            for (RopeNode& node2 : node1.childs) {
                Float im1 = node1.mass == 0.0f ? 0.0f : 1.0f / node1.mass;
                Float im2 = node2.mass == 0.0f ? 0.0f : 1.0f / node2.mass;
                Float mult1 = im1 + im2 == 0.0f ? 0.0f : im1 / (im1 + im2), mult2 = im1 + im2 == 0.0f ? 0.0f : im2 / (im1 + im2);

                Vector2 diff = node1.position - node2.position;

                Float dist = diff.length();
                Float difference = 0.0f;
                if (dist > 0.0f)
                    difference = (dist - RopeNodeDistance) / dist;

                diff *= 0.5f * difference;

                node1.position -= diff * mult1;
                node2.position += diff * mult2;
            }
        }
    }

    RopeNode* getClosestNode(const Magnum::Vector2& point) {
        Float distance = std::numeric_limits<Float>::max();
        RopeNode* result = nullptr;

        for (auto& node : _nodes) {
            auto nodeDistance = (node.position - point).length();
            if (nodeDistance < distance) {
                distance = nodeDistance;
                result = &node;
            }
        }

        return result;
    }

    struct RopeNode {
        RopeNode(Vector2&& p) : position{ p }, positionOld{ std::move(p) } {}

        Vector2 position;
        Vector2 positionOld;
        Float mass = 1.0f;
        std::vector<std::reference_wrapper<RopeNode>> childs;
    };

    std::vector<RopeNode> _nodes;
    size_t _pointsX;
    size_t _pointsY;
};

struct InstanceData {
    Matrix3 transformation;
    Color3 color;
};

struct DrawMesh {
    void create(const Magnum::Trade::MeshData& data) {
        instanceBuffer = GL::Buffer{};
        mesh = MeshTools::compile(data);
        mesh.addVertexBufferInstanced(instanceBuffer, 1, 0, Shaders::FlatGL2D::TransformationMatrix{}, Shaders::FlatGL2D::Color3{});
    }

    void draw(Magnum::Shaders::FlatGL2D& shader, Matrix3& projection) {
        if (instanceData.empty())
            return;

        instanceBuffer.setData({ instanceData.data(), instanceData.size() }, GL::BufferUsage::DynamicDraw);
        mesh.setInstanceCount(instanceData.size());
        shader.setTransformationProjectionMatrix(projection).draw(mesh);
    }

    GL::Mesh mesh{ NoCreate };
    GL::Buffer instanceBuffer{ NoCreate };
    std::vector<InstanceData> instanceData;
};

class RopeSimulation2DExample: public Platform::Application {
    public:
        explicit RopeSimulation2DExample(const Arguments& arguments);

    protected:
        void viewportEvent(ViewportEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;
        void textInputEvent(TextInputEvent& event) override;
        void drawEvent() override;

        Vector2 windowPos2WorldPos(const Vector2i& winPos);
        Matrix3 createTransformation(Vector2 translation, Float radians, Vector2 scale);
        void drawLines(const std::vector<Vector2>& points, Color3 color);
        void drawPoints(const std::vector<Vector2>& points, Color3 color);
        void drawRope();
        void simulateRope();
        Rope createRope();
        void setupWindow();
        void setupCamera();
        void setupGui();
        void drawGui();
        void showMenu();
        void interactionGrab();
        void interactionCut(const Vector2& newMousePosition);

        bool _showMenu = true;
        ImGuiIntegration::Context _imGuiContext{ NoCreate };

        Matrix3 _cameraProjection;
        Vector2 _cameraSize;
        Shaders::FlatGL2D _shader{ NoCreate };
        Shaders::FlatGL2D _shaderInstanced{ NoCreate };
        GL::Mesh _meshLines{ NoCreate };
        DrawMesh _meshCircle;

        enum class RopeInteraction : int32_t { Cut = 0, Grab = 1, Attract = 2 };
        RopeInteraction _ropeInteraction = RopeInteraction::Cut;
        Rope::RopeNode* _grabbedNode = nullptr;
        Rope _rope;

        Vector2 _mousePosition;
        bool _isMousePressed = false;
};

RopeSimulation2DExample::RopeSimulation2DExample(const Arguments& arguments)
    : Platform::Application{ arguments, NoCreate }, _rope{ createRope()} {
    setupWindow();
    setupGui();
    setupCamera();

    setSwapInterval(1);
    setMinimalLoopPeriod(16);
}

void RopeSimulation2DExample::setupWindow() {
    const Vector2 dpiScaling = this->dpiScaling({});
    Configuration conf;
    conf.setTitle("Magnum 2D Rope Simulation Example")
        .setSize(conf.size(), dpiScaling)
        .setWindowFlags(Configuration::WindowFlag::Resizable);
    GLConfiguration glConf;
    glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);
    if (!tryCreate(conf, glConf))
        create(conf, glConf.setSampleCount(0));

    _shader = Shaders::FlatGL2D();

    auto shaderConfig = Shaders::FlatGL2D::Configuration{};
    shaderConfig.setFlags(Shaders::FlatGL2D::Flag::VertexColor | Shaders::FlatGL2D::Flag::InstancedTransformation);
    _shaderInstanced = Shaders::FlatGL2D{ shaderConfig };

    _meshLines = GL::Mesh(MeshPrimitive::Lines);
    _meshCircle.create(Primitives::circle2DSolid(20));
}

void RopeSimulation2DExample::setupCamera() {
    Vector2 winSize = { (Float)windowSize().x(), (Float)windowSize().y() };
    _cameraSize = winSize / 40.0f;
    _cameraProjection = Matrix3::projection(_cameraSize);
}
void RopeSimulation2DExample::setupGui() {
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    _imGuiContext = ImGuiIntegration::Context{ *ImGui::GetCurrentContext(), Vector2{windowSize()} / dpiScaling(), windowSize(), framebufferSize() };

    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

void RopeSimulation2DExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    simulateRope();
    drawRope();

    if (_isMousePressed && _ropeInteraction == RopeInteraction::Grab)
        interactionGrab();

    drawGui();

    swapBuffers();
    redraw();
}

void RopeSimulation2DExample::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
    _imGuiContext.relayout(Vector2{event.windowSize()}/event.dpiScaling(), event.windowSize(), event.framebufferSize());
}

void RopeSimulation2DExample::keyPressEvent(KeyEvent& event) {
    switch(event.key()) {
        case KeyEvent::Key::H:
            _showMenu ^= true;
            event.setAccepted(true);
            break;
        default:
            if(_imGuiContext.handleKeyPressEvent(event))
                event.setAccepted(true);
    }
}

void RopeSimulation2DExample::keyReleaseEvent(KeyEvent& event) {
    if(_imGuiContext.handleKeyReleaseEvent(event)) {
        event.setAccepted(true);
        return;
    }
}

void RopeSimulation2DExample::mousePressEvent(MouseEvent& event) {
    if(_imGuiContext.handleMousePressEvent(event)) {
        event.setAccepted(true);
        return;
    }

    _isMousePressed = true;

    if (_ropeInteraction == RopeInteraction::Grab)
        _grabbedNode = _rope.getClosestNode(windowPos2WorldPos(event.position()));
}

void RopeSimulation2DExample::mouseReleaseEvent(MouseEvent& event) {
    if (_imGuiContext.handleMouseReleaseEvent(event))
        event.setAccepted(true);

    _isMousePressed = false;
    _grabbedNode = nullptr;
}

void RopeSimulation2DExample::interactionGrab() {
    if (_grabbedNode) {
        _grabbedNode->position = _mousePosition;
        _grabbedNode->positionOld = _mousePosition;
    }
}

void RopeSimulation2DExample::interactionCut(const Vector2& newMousePosition) {
    for (auto& node : _rope._nodes) {
        for (auto it = node.childs.begin(); it != node.childs.end();) {
            if (Utils::doLineSegmentsIntersect(_mousePosition, newMousePosition, node.position, ((Rope::RopeNode&)(*it)).position))
                it = node.childs.erase(it);
            else
                it++;
        }
    }
}

void RopeSimulation2DExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(_imGuiContext.handleMouseMoveEvent(event)) {
        event.setAccepted(true);
        return;
    }

    auto newMousePosition = windowPos2WorldPos(event.position());
    
    if (_isMousePressed && _ropeInteraction == RopeInteraction::Cut)
        interactionCut(newMousePosition);

    _mousePosition = newMousePosition;
}

void RopeSimulation2DExample::mouseScrollEvent(MouseScrollEvent& event) {
    const Float delta = event.offset().y();
    if(Math::abs(delta) < 1.0e-2f)
        return;

    if(_imGuiContext.handleMouseScrollEvent(event)) {
        event.setAccepted();
        return;
    }
}

void RopeSimulation2DExample::textInputEvent(TextInputEvent& event) {
    if(_imGuiContext.handleTextInputEvent(event))
        event.setAccepted(true);
}

void RopeSimulation2DExample::showMenu() {
    ImGui::SetNextWindowPos({ 10.0f, 10.0f }, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.5f);

    ImGui::Begin("Options", nullptr);

    ImGui::Text("Hide/show menu: H");
    ImGui::Text("Rendering: %3.2f FPS", Double(ImGui::GetIO().Framerate));
    ImGui::Spacing();

    if(ImGui::TreeNode("Simulation")) {
        ImGui::PushID("Simulation");
        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
        ImGui::SliderFloat("Time Delta", &RopeSimulationDelta, 0.001f, 0.030f);
        ImGui::SliderFloat("Gravity", &RopeGravity.y(), -15.0f, -0.1f);
        ImGui::SliderInt("Constraint Iterations", &RopeConstraintIterations, 1, 40);
        ImGui::PopID();
        ImGui::TreePop();
    }
    ImGui::Spacing();
    ImGui::Separator();

    if(ImGui::TreeNodeEx("Rope", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Rope");
        ImGui::PushItemWidth(ImGui::GetWindowWidth()*0.5f);
        if (ImGui::SliderFloat("Node Distance", &RopeNodeDistance, 0.1f, 2.0f))
            _rope = createRope();
        ImGui::Combo("Interaction", (int*)&_ropeInteraction, "Cut\0Grab\0Attract\0");
        ImGui::PopID();
        ImGui::TreePop();
    }
    ImGui::Spacing();
    ImGui::Separator();

    ImGui::End();
}

Vector2 RopeSimulation2DExample::windowPos2WorldPos(const Vector2i& windowPosition) {
    Vector2 result = -_cameraSize / 2.0f;

    result.x() += (windowPosition.x() / (Float)windowSize().x()) * _cameraSize.x();
    result.y() += ((windowSize().y() - windowPosition.y()) / (Float)windowSize().y()) * _cameraSize.y();

    return result;
}

void RopeSimulation2DExample::drawLines(const std::vector<Vector2>& points, Color3 color) {
    GL::Buffer vertices;
    vertices.setData({ points.data(), points.size() }, GL::BufferUsage::StaticDraw);

    _meshLines.addVertexBuffer(vertices, 0, Shaders::FlatGL2D::Position{});
    _meshLines.setCount(points.size());

    _shader.setColor(color).setTransformationProjectionMatrix(_cameraProjection).draw(_meshLines);
}

Matrix3 RopeSimulation2DExample::createTransformation(Vector2 translation, Float radians, Vector2 scale) {
    auto rotation = Math::Complex<Float>::rotation(Math::Rad<Float>(radians));

    return Matrix3::from(rotation.toMatrix(), translation) * Matrix3::scaling(scale);
}

void RopeSimulation2DExample::drawPoints(const std::vector<Vector2>& points, Color3 color){
    for (auto& point : points)
        _meshCircle.instanceData.push_back({ createTransformation(point, 0.0f, { 0.08f, 0.08f }), color });

    _meshCircle.draw(_shaderInstanced, _cameraProjection);
    _meshCircle.instanceData.clear();
}

Rope RopeSimulation2DExample::createRope() {
    return Rope({ -8.0f, -6.0f }, { 8.0f, 7.4f });
}

void RopeSimulation2DExample::drawRope() {
    drawLines(_rope.getLinePairs(), 0x2f83cc_rgbf);
    drawPoints(_rope.getLinePoints(), 0x2f83cc_rgbf);
}

void RopeSimulation2DExample::simulateRope() {
    _rope.simulate(_isMousePressed && _ropeInteraction == RopeInteraction::Attract ? _mousePosition : Corrade::Containers::Optional<Vector2>{});
}

void RopeSimulation2DExample::drawGui() {
    _imGuiContext.newFrame();

    if (ImGui::GetIO().WantTextInput && !isTextInputActive())
        startTextInput();
    else if (!ImGui::GetIO().WantTextInput && isTextInputActive())
        stopTextInput();

    if (_showMenu)
        showMenu();

    _imGuiContext.updateApplicationCursor(*this);

    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);

    _imGuiContext.drawFrame();

    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::RopeSimulation2DExample)
