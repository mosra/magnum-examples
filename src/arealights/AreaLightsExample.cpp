/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
            Vladimír Vondruš <mosra@centrum.cz>
        2017 — Jonathan Hale <squareys@googlemail.com>, based on "Real-Time
            Polygonal-Light Shading with Linearly Transformed Cosines", by Eric
            Heitz et al, https://eheitzresearch.wordpress.com/415-2/

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
#include <Corrade/Containers/Reference.h>
#include <Corrade/Interconnect/Receiver.h>
#include <Corrade/PluginManager/PluginManager.h>
#include <Corrade/Utility/FormatStl.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData3D.h>
#include <Magnum/Ui/Anchor.h>
#include <Magnum/Ui/Button.h>
#include <Magnum/Ui/Label.h>
#include <Magnum/Ui/Plane.h>
#include <Magnum/Ui/UserInterface.h>
#include <Magnum/Ui/ValidatedInput.h>

#ifdef MAGNUM_TARGET_GLES
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Half.h>
#endif

namespace Magnum { namespace Examples {

/* Class for the area light shader */
class AreaLightShader: public GL::AbstractShaderProgram {
    public:
        explicit AreaLightShader(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}

        explicit AreaLightShader() {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GLES300);

            /* Load and compile shaders from compiled-in resource */
            Utility::Resource rs("arealights-data");

            GL::Shader vert{GL::Version::GLES300, GL::Shader::Type::Vertex};
            GL::Shader frag{GL::Version::GLES300, GL::Shader::Type::Fragment};

            vert.addSource(rs.get("AreaLights.vert"));
            frag.addSource(rs.get("AreaLights.frag"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({vert, frag}));

            attachShaders({vert, frag});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            setUniform(uniformLocation("s_texLTCMat"), LtcMatTextureUnit);
            setUniform(uniformLocation("s_texLTCAmp"), LtcAmpTextureUnit);

            /* Get uniform locations */
            _transformationMatrixUniform = uniformLocation("u_transformationMatrix");
            _projectionMatrixUniform = uniformLocation("u_projectionMatrix");
            _viewMatrixUniform = uniformLocation("u_viewMatrix");
            _normalMatrixUniform = uniformLocation("u_normalMatrix");
            _viewPositionUniform = uniformLocation("u_viewPosition");

            _baseColorUniform = uniformLocation("u_baseColor");
            _metalnessUniform = uniformLocation("u_metalness");
            _roughnessUniform = uniformLocation("u_roughness");
            _f0Uniform = uniformLocation("u_f0");

            _lightIntensityUniform = uniformLocation("u_lightIntensity");
            _twoSidedUniform = uniformLocation("u_twoSided");
            _lightQuadUniform = uniformLocation("u_quadPoints");
        }

        /* Matrices: view, transformation, projection, normal */

        AreaLightShader& setTransformationMatrix(const Matrix4& matrix) {
            setUniform(_transformationMatrixUniform, matrix);
            return *this;
        }

        AreaLightShader& setProjectionMatrix(const Matrix4& matrix) {
            setUniform(_projectionMatrixUniform, matrix);
            return *this;
        }

        AreaLightShader& setViewMatrix(const Matrix4& matrix) {
            setUniform(_viewMatrixUniform, matrix);
            return *this;
        }

        AreaLightShader& setNormalMatrix(const Matrix3& matrix) {
            setUniform(_normalMatrixUniform, matrix);
            return *this;
        }

        /* View position */

        AreaLightShader& setViewPosition(const Vector3& pos) {
            setUniform(_viewPositionUniform, pos);
            return *this;
        }

        /* Material properties */

        AreaLightShader& setBaseColor(const Color3& color) {
            setUniform(_baseColorUniform, color);
            return *this;
        }

        AreaLightShader& setMetalness(Float v) {
            setUniform(_metalnessUniform, v);
            return *this;
        }

        AreaLightShader& setRoughness(Float v) {
            setUniform(_roughnessUniform, v);
            return *this;
        }

        AreaLightShader& setF0(Float f0) {
            setUniform(_f0Uniform, f0);
            return *this;
        }

        /* Light properties */

        AreaLightShader& setLightIntensity(Float i) {
            setUniform(_lightIntensityUniform, i);
            return *this;
        }

        AreaLightShader& setTwoSided(bool b) {
            setUniform(_twoSidedUniform, b ? 1.0f : 0.0f);
            return *this;
        }

        AreaLightShader& setLightQuad(Containers::StaticArrayView<4, Vector3> quadPoints) {
            setUniform(_lightQuadUniform, quadPoints);
            return *this;
        }

        /* LTC lookup textures */

        AreaLightShader& bindTextures(GL::Texture2D& ltcMat, GL::Texture2D& ltcAmp) {
            GL::Texture2D::bind(LtcMatTextureUnit, {&ltcMat, &ltcAmp});
            return *this;
        }

    private:
        enum: Int {
            LtcMatTextureUnit = 0,
            LtcAmpTextureUnit = 1
        };

        Int _transformationMatrixUniform,
            _projectionMatrixUniform,
            _viewMatrixUniform,
            _normalMatrixUniform,
            _viewPositionUniform,
            _baseColorUniform,
            _metalnessUniform,
            _roughnessUniform,
            _f0Uniform,
            _lightIntensityUniform,
            _twoSidedUniform,
            _lightQuadUniform;
};

/* Base UI plane */
constexpr Vector2 WidgetSize{80, 32};
const std::regex FloatValidator{R"(-?\d+(\.\d+)?)"};

struct BaseUiPlane: Ui::Plane {
    explicit BaseUiPlane(Ui::UserInterface& ui):
        Ui::Plane{ui, Ui::Snap::Top|Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::Right, 0, 16, 128},
        metalness{*this, {Ui::Snap::Top|Ui::Snap::Right, WidgetSize}, FloatValidator, "0.5", 5},
        roughness{*this, {Ui::Snap::Bottom, metalness, WidgetSize}, FloatValidator, "0.25", 5},
        f0{*this, {Ui::Snap::Bottom, roughness, WidgetSize}, FloatValidator, "0.25", 5},

        apply{*this, {Ui::Snap::Bottom|Ui::Snap::Right, WidgetSize}, "Apply", Ui::Style::Primary},
        reset{*this, {Ui::Snap::Top, apply, WidgetSize}, "Reset", Ui::Style::Danger}
    {
        Ui::Label{*this, {Ui::Snap::Left, metalness}, "Metalness", Text::Alignment::MiddleRight};
        Ui::Label{*this, {Ui::Snap::Left, roughness}, "Roughness", Text::Alignment::MiddleRight};
        Ui::Label{*this, {Ui::Snap::Left, f0}, "ƒ₀", Text::Alignment::MiddleRight};

        Ui::Label{*this, {Ui::Snap::Bottom|Ui::Snap::Left, WidgetSize},
            "Use WASD + mouse to move, (Shift +) M/R/F to change parameters.",
            Text::Alignment::MiddleLeft};
    }

    Ui::ValidatedInput metalness,
        roughness,
        f0;
    Ui::Button apply,
        reset;
};

using namespace Math::Literals;

class AreaLightsExample: public Platform::Application, public Interconnect::Receiver {
    public:
        explicit AreaLightsExample(const Arguments& arguments);

        void enableApplyButton(const std::string&);
        void apply();
        void reset();

    private:
        void drawEvent() override;

        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;
        void textInputEvent(TextInputEvent& event) override;

        Matrix4 _lightTransform[3]{
            Matrix4::translation({-3.25f, 2.0f, -2.1f})*
            Matrix4::rotationY(30.0_degf)*
            Matrix4::rotationX(-15.0_degf)*
            Matrix4::rotationZ(10.0_degf)*
            Matrix4::scaling({0.5f, 1.75f, 1.0f}),

            Matrix4::translation({0.0f, 1.8f, -2.5f})*
            Matrix4::rotationX(-5.0_degf)*
            Matrix4::rotationZ(30.0_degf)*
            Matrix4::scaling({1.25f, 1.25f, 1.0f}),

            Matrix4::translation({3.25f, 2.0f, -2.1f})*
            Matrix4::rotationY(-30.0_degf)*
            Matrix4::rotationX(5.0_degf)*
            Matrix4::rotationZ(-10.0_degf)*
            Matrix4::scaling({0.75f, 1.25f, 1.0f})};

        Color3 _lightColor[3]{0xc7cf2f_rgbf, 0x2f83cc_rgbf, 0x3bd267_rgbf};

        Float _lightIntensity[3]{1.0f, 1.5f, 1.0f};

        bool _lightTwoSided[3]{true, false, true};

        /* Plane mesh */
        GL::Buffer _vertices{NoCreate};
        GL::Mesh _plane{NoCreate};

        /* Shaders */
        AreaLightShader _areaLightShader{NoCreate};
        Shaders::Flat3D _flatShader{NoCreate};

        /* Look Up Textures for arealights shader */
        GL::Texture2D _ltcAmp{NoCreate};
        GL::Texture2D _ltcMat{NoCreate};

        /* Camera and interaction */
        Matrix4 _transformation, _projection, _view;
        Vector2i _previousMousePosition;

        Vector3 _cameraPosition{0.0f, 1.0f, 7.6f};
        Vector3 _cameraDirection;
        Vector2 _cameraRotation;

        /* Material properties */
        Float _metalness = 0.5f;
        Float _roughness = 0.25f;
        Float _f0 = 0.5f; /* Specular reflection coefficient */

        /* UI */
        Containers::Optional<Ui::UserInterface> _ui;
        Containers::Optional<BaseUiPlane> _baseUiPlane;
};

constexpr struct {
    Vector3 position;
    Vector3 normal;
} LightVertices[] = {
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
};

AreaLightsExample::AreaLightsExample(const Arguments& arguments): Platform::Application{arguments, NoCreate} {
    /* Try to create multisampled context, but be nice and fall back if not
       available. Enable only 2x MSAA if we have enough DPI. */
    {
        const Vector2 dpiScaling = this->dpiScaling({});
        Configuration conf;
        conf.setTitle("Magnum Area Lights Example")
            .setSize(conf.size(), dpiScaling);
        GLConfiguration glConf;
        glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);
        if(!tryCreate(conf, glConf))
            create(conf, glConf.setSampleCount(0));
    }

    /* Make it all DARK, eanble face culling so one-sided lights are properly
       visualized */
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setClearColor(0x000000_rgbf);

    /* Setup the plane mesh, which will be used for both the floor and light
       visualization */
    _vertices = GL::Buffer{};
    _vertices.setData(LightVertices, GL::BufferUsage::StaticDraw);
    _plane = GL::Mesh{};
    _plane.setPrimitive(GL::MeshPrimitive::TriangleFan)
        .addVertexBuffer(_vertices, 0, Shaders::Generic3D::Position{}, Shaders::Generic3D::Normal{})
        .setCount(Containers::arraySize(LightVertices));

    /* Setup project and floor plane tranformation matrix */
    _projection = Matrix4::perspectiveProjection(60.0_degf, 4.0f/3.0f, 0.1f, 50.0f);
    _transformation = Matrix4::rotationX(-90.0_degf)*Matrix4::scaling(Vector3{25.0f});

    /* Load LTC matrix and BRDF textures */
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate("DdsImporter");
    if(!importer) std::exit(1);

    const Utility::Resource rs{"arealights-data"};
    if(!importer->openData(rs.getRaw("ltc_amp.dds")))
        std::exit(2);

    /* Set texture data and parameters */
    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    _ltcAmp = GL::Texture2D{};
    _ltcAmp.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear);

    /* Convert to half-float in case we can't filter float textures */
    #ifdef MAGNUM_TARGET_GLES
    if(!GL::Context::current().isExtensionSupported<GL::Extensions::OES::texture_float_linear>()) {
        CORRADE_INTERNAL_ASSERT(!(image->size().x()%2)); /* so we don't have to think about alignment */
        auto floats = Containers::arrayCast<Float>(image->data());
        Containers::Array<Half> halves{std::size_t(image->size().product()*2)};
        for(std::size_t i = 0; i != floats.size(); ++i)
            halves[i] = Half{floats[i]};
        _ltcAmp.setStorage(1, GL::TextureFormat::RG16F, image->size())
            .setSubImage(0, {}, ImageView2D{PixelFormat::RG16F, image->size(), halves});
    } else
    #endif
    {
        _ltcAmp.setStorage(1, GL::TextureFormat::RG32F, image->size())
            .setSubImage(0, {}, *image);
    }

    if(!importer->openData(rs.getRaw("ltc_mat.dds")))
        std::exit(2);

    /* Set texture data and parameters */
    image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    _ltcMat = GL::Texture2D{};
    _ltcMat.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear);

    /* Convert to half-float in case we can't filter float textures */
    #ifdef MAGNUM_TARGET_GLES
    if(!GL::Context::current().isExtensionSupported<GL::Extensions::OES::texture_float_linear>()) {
        auto floats = Containers::arrayCast<Float>(image->data());
        Containers::Array<Half> halves{std::size_t(image->size().product()*4)};
        for(std::size_t i = 0; i != floats.size(); ++i)
            halves[i] = Half{floats[i]};
        _ltcMat.setStorage(1, GL::TextureFormat::RGBA16F, image->size())
            .setSubImage(0, {}, ImageView2D{PixelFormat::RGBA16F, image->size(), halves});
    } else
    #endif
    {
        _ltcMat.setStorage(1, GL::TextureFormat::RGBA32F, image->size())
            .setSubImage(0, {}, *image);
    }

    /* Compile shaders */
    _areaLightShader = AreaLightShader{};
    _flatShader = Shaders::Flat3D{};

    /* Create the UI */
    _ui.emplace(Vector2{windowSize()}/dpiScaling(), windowSize(), framebufferSize(), Ui::mcssDarkStyleConfiguration(), "ƒ₀");
    Interconnect::connect(*_ui, &Ui::UserInterface::inputWidgetFocused, *this, &AreaLightsExample::startTextInput);
    Interconnect::connect(*_ui, &Ui::UserInterface::inputWidgetBlurred, *this, &AreaLightsExample::stopTextInput);

    /* Base UI plane */
    _baseUiPlane.emplace(*_ui);
    Interconnect::connect(_baseUiPlane->metalness, &Ui::Input::valueChanged, *this, &AreaLightsExample::enableApplyButton);
    Interconnect::connect(_baseUiPlane->roughness, &Ui::Input::valueChanged, *this, &AreaLightsExample::enableApplyButton);
    Interconnect::connect(_baseUiPlane->f0, &Ui::Input::valueChanged, *this, &AreaLightsExample::enableApplyButton);
    Interconnect::connect(_baseUiPlane->apply, &Ui::Button::tapped, *this, &AreaLightsExample::apply);
    Interconnect::connect(_baseUiPlane->reset, &Ui::Button::tapped, *this, &AreaLightsExample::reset);

    /* Apply the default values */
    apply();
}

void AreaLightsExample::enableApplyButton(const std::string&) {
    _baseUiPlane->apply.setEnabled(Ui::ValidatedInput::allValid({
        _baseUiPlane->metalness,
        _baseUiPlane->roughness,
        _baseUiPlane->f0}));
}

void AreaLightsExample::apply() {
    _metalness = Math::clamp(std::stof(_baseUiPlane->metalness.value()), 0.1f, 1.0f);
    _roughness = Math::clamp(std::stof(_baseUiPlane->roughness.value()), 0.1f, 1.0f);
    _f0 = Math::clamp(std::stof(_baseUiPlane->f0.value()), 0.1f, 1.0f);

    _areaLightShader.setMetalness(_metalness)
        .setRoughness(_roughness)
        .setF0(_f0);

    /* Set the clamped values back */
    _baseUiPlane->metalness.setValue(Utility::formatString("{:.5}", _metalness));
    _baseUiPlane->roughness.setValue(Utility::formatString("{:.5}", _roughness));
    _baseUiPlane->f0.setValue(Utility::formatString("{:.5}", _f0));
}

void AreaLightsExample::reset() {
    _baseUiPlane->metalness.setValue("0.5");
    _baseUiPlane->roughness.setValue("0.25");
    _baseUiPlane->f0.setValue("0.25");

    _cameraRotation = {};
    _cameraPosition = {0.0f, 1.0f, 6.0f};

    apply();
}

void AreaLightsExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    /* Update view matrix */
    _cameraPosition += _cameraDirection;
    _view = Matrix4::rotationX(Rad{_cameraRotation.y()})*
            Matrix4::rotationY(Rad{_cameraRotation.x()})*
            Matrix4::translation(-_cameraPosition);

    /* Draw light on the floor. Cheat a bit and just add everything together,
       enabling depth test for the first only. Will work as long as the
       background is black. */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::One);
    _areaLightShader.bindTextures(_ltcMat, _ltcAmp);
    for(std::size_t i: {0, 1, 2}) {
        Vector3 quadPoints[4];
        for(std::size_t p: {0, 1, 2, 3})
            quadPoints[p] = _lightTransform[i].transformPoint(LightVertices[p].position);

        _areaLightShader
            .setTransformationMatrix(_transformation)
            .setProjectionMatrix(_projection)
            .setViewMatrix(_view)
            .setNormalMatrix(_transformation.rotationScaling())
            .setViewPosition(_view.invertedRigid().translation())
            .setLightQuad(quadPoints)
            .setBaseColor(_lightColor[i])
            .setLightIntensity(_lightIntensity[i])
            .setTwoSided(_lightTwoSided[i]);

        if(i == 0)
            GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

        _plane.draw(_areaLightShader);

        if(i == 0)
            GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    }
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    /* Draw light visualization, this time with depth test enabled for all.
       Draw twice for two-sided lights. */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    for(std::size_t i: {0, 1, 2}) {
        _flatShader.setColor(_lightColor[i]*_lightIntensity[i]*1.25f)
            .setTransformationProjectionMatrix(_projection*_view*_lightTransform[i]);
        _plane.draw(_flatShader);

        if(_lightTwoSided[i]) {
            _flatShader.setTransformationProjectionMatrix(_projection*_view*_lightTransform[i]*Matrix4::scaling(Vector3::xScale(-1.0f)));
            _plane.draw(_flatShader);
        }
    }
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

    /* Draw the UI */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    _ui->draw();
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::One);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    /* Redraw only if moving somewhere */
    swapBuffers();
    if(!_cameraDirection.isZero()) redraw();
}

void AreaLightsExample::mousePressEvent(MouseEvent& event) {
    if((event.button() == MouseEvent::Button::Left))
        _previousMousePosition = event.position();

    if(!_ui->handlePressEvent(event.position())) return;

    redraw();
}

void AreaLightsExample::mouseReleaseEvent(MouseEvent& event) {
    if(_ui->handleReleaseEvent(event.position()))
        redraw();
}

void AreaLightsExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(_ui->handleMoveEvent(event.position())) {
        /* UI handles it */

    } else if((event.buttons() & MouseMoveEvent::Button::Left)) {
        const Vector2 delta = 3.0f*
            Vector2{event.position() - _previousMousePosition}/Vector2{GL::defaultFramebuffer.viewport().size()};
        _cameraRotation += delta;

        _previousMousePosition = event.position();

    } else return;

    redraw();
}

void AreaLightsExample::keyPressEvent(KeyEvent& event) {
    /* If an input is focused, pass the events only to the UI */
    if(isTextInputActive() && _ui->focusedInputWidget()) {
        if(!_ui->focusedInputWidget()->handleKeyPress(event)) return;

    /* Movement */
    } else if(event.key() == KeyEvent::Key::W) {
        _cameraDirection = -_view.inverted().backward()*0.1f;
    } else if(event.key() == KeyEvent::Key::S) {
        _cameraDirection = _view.inverted().backward()*0.1f;
    } else if (event.key() == KeyEvent::Key::A) {
        _cameraDirection = Math::cross(_view.inverted().backward(), {0.0f, 1.0f, 0.0})*0.1f;
    } else if (event.key() == KeyEvent::Key::D) {
        _cameraDirection = -Math::cross(_view.inverted().backward(), { 0.0f, 1.0f, 0.0 })*0.1f;

    /* Increase/decrease roughness */
    } else if(event.key() == KeyEvent::Key::R) {
        _roughness = Math::clamp(
            _roughness + 0.01f*(event.modifiers() & KeyEvent::Modifier::Shift ? -1 : 1),
            0.1f, 1.0f);
        _areaLightShader.setRoughness(_roughness);
        _baseUiPlane->roughness.setValue(Utility::formatString("{:.5}", _roughness));

    /* Increase/decrease metalness */
    } else if(event.key() == KeyEvent::Key::M) {
        _metalness = Math::clamp(
            _metalness + 0.01f*(event.modifiers() & KeyEvent::Modifier::Shift ? -1 : 1),
            0.1f, 1.0f);
        _areaLightShader.setMetalness(_metalness);
        _baseUiPlane->metalness.setValue(Utility::formatString("{:.5}", _metalness));

    /* Increase/decrease f0 */
    } else if(event.key() == KeyEvent::Key::F) {
        _f0 = Math::clamp(
            _f0 + 0.01f*(event.modifiers() & KeyEvent::Modifier::Shift ? -1 : 1),
            0.1f, 1.0f);
        _areaLightShader.setF0(_f0);
        _baseUiPlane->f0.setValue(Utility::formatString("{:.5}", _f0));

    /* Reload shader */
    } else if(event.key() == KeyEvent::Key::F5) {
        #ifdef CORRADE_IS_DEBUG_BUILD
        Utility::Resource::overrideGroup("arealights-data", "../src/arealights/resources.conf");
        _areaLightShader = AreaLightShader{};
        #endif

    } else return;

    redraw();
}

void AreaLightsExample::keyReleaseEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::W || event.key() == KeyEvent::Key::S ||
       event.key() == KeyEvent::Key::A || event.key() == KeyEvent::Key::D)
        _cameraDirection = {};
    else return;

    redraw();
}

void AreaLightsExample::textInputEvent(TextInputEvent& event) {
    if(isTextInputActive() && _ui->focusedInputWidget() && _ui->focusedInputWidget()->handleTextInput(event))
        redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::AreaLightsExample)
