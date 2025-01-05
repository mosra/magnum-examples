/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
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
#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/Containers/StringStl.h>
#include <Corrade/PluginManager/PluginManager.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/FormatStl.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/ImageView.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
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
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Text/AbstractFont.h> /** @todo remove once extra glyph cache fill is done better */
#include <Magnum/Text/AbstractGlyphCache.h> /** @todo remove once extra glyph cache fill is done better */
#include <Magnum/Text/Alignment.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/Ui/Anchor.h>
#include <Magnum/Ui/Application.h>
#include <Magnum/Ui/Button.h>
#include <Magnum/Ui/EventLayer.h>
#include <Magnum/Ui/Input.h>
#include <Magnum/Ui/Label.h>
#include <Magnum/Ui/SnapLayouter.h>
#include <Magnum/Ui/Style.h>
#include <Magnum/Ui/TextLayer.h> /** @todo remove once extra glyph cache fill is done better */
#include <Magnum/Ui/TextProperties.h>
#include <Magnum/Ui/UserInterfaceGL.h>

namespace Magnum { namespace Examples {

/* Class for the area light shader */
class AreaLightShader: public GL::AbstractShaderProgram {
    public:
        explicit AreaLightShader(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}

        explicit AreaLightShader() {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL430);

            /* Load and compile shaders from compiled-in resource */
            Utility::Resource rs("arealights-data");

            GL::Shader vert{GL::Version::GL430, GL::Shader::Type::Vertex};
            GL::Shader frag{GL::Version::GL430, GL::Shader::Type::Fragment};

            vert.addSource(Utility::format(
                    "#define POSITION_ATTRIBUTE_LOCATION {}\n"
                    "#define NORMAL_ATTRIBUTE_LOCATION {}\n",
                    Shaders::GenericGL3D::Position::Location,
                    Shaders::GenericGL3D::Normal::Location))
                .addSource(rs.getString("AreaLights.vert"));
            frag.addSource(rs.getString("AreaLights.frag"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile() && frag.compile());

            attachShaders({vert, frag});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

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

using namespace Math::Literals;

class AreaLightsExample: public Platform::Application {
    public:
        explicit AreaLightsExample(const Arguments& arguments);

        void enableApplyButton(const std::string&);
        void apply();
        void reset();

    private:
        void drawEvent() override;

        void pointerPressEvent(PointerEvent& event) override;
        void pointerReleaseEvent(PointerEvent& event) override;
        void pointerMoveEvent(PointerMoveEvent& event) override;
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
        Shaders::FlatGL3D _flatShader{NoCreate};

        /* Look Up Textures for arealights shader */
        GL::Texture2D _ltcAmp{NoCreate};
        GL::Texture2D _ltcMat{NoCreate};

        /* Camera and interaction */
        Matrix4 _transformation, _projection, _view;
        Vector2 _previousPointerPosition;

        Vector3 _cameraPosition{0.0f, 1.0f, 7.6f};
        Vector3 _cameraDirection;
        Vector2 _cameraRotation;

        /* Material properties */
        Float _metalness = 0.5f;
        Float _roughness = 0.25f;
        Float _f0 = 0.5f; /* Specular reflection coefficient */

        /* UI */
        struct {
            Ui::UserInterfaceGL ui{NoCreate};
            Ui::Input metalness{NoCreate, ui},
                roughness{NoCreate, ui},
                f0{NoCreate, ui};
        } _ui;
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
        .addVertexBuffer(_vertices, 0, Shaders::GenericGL3D::Position{}, Shaders::GenericGL3D::Normal{})
        .setCount(Containers::arraySize(LightVertices));

    /* Setup project and floor plane tranformation matrix */
    _projection = Matrix4::perspectiveProjection(60.0_degf, 4.0f/3.0f, 0.1f, 50.0f);
    _transformation = Matrix4::rotationX(-90.0_degf)*Matrix4::scaling(Vector3{25.0f});

    /* Load LTC matrix and BRDF textures. The shader assumes the data is
       Y-down, so disable Y-flipping in the importer. */
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate("DdsImporter");
    if(!importer) std::exit(1);
    importer->configuration().setValue("assumeYUpZBackward", true);

    const Utility::Resource rs{"arealights-data"};
    if(!importer->openData(rs.getRaw("ltc_amp.dds")))
        std::exit(2);

    /* Set texture data and parameters */
    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    _ltcAmp = GL::Texture2D{};
    _ltcAmp.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::TextureFormat::RG32F, image->size())
        .setSubImage(0, {}, *image);

    if(!importer->openData(rs.getRaw("ltc_mat.dds")))
        std::exit(2);

    /* Set texture data and parameters */
    image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    _ltcMat = GL::Texture2D{};
    _ltcMat.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::TextureFormat::RGBA32F, image->size())
        .setSubImage(0, {}, *image);

    /* Compile shaders */
    _areaLightShader = AreaLightShader{};
    _flatShader = Shaders::FlatGL3D{};

    /* Create the UI */
    {
        _ui.ui.create(Vector2{windowSize()}/dpiScaling(), Vector2{windowSize()}, framebufferSize(), Ui::McssDarkStyle{});
        /** @todo make a builtin API for this, or, better, make it automatic */
        CORRADE_INTERNAL_ASSERT(_ui.ui.textLayer().shared().font(Ui::fontHandle(1, 1)).fillGlyphCache(_ui.ui.textLayer().shared().glyphCache(), "ƒ₀"));
        Ui::NodeHandle root = Ui::snap(_ui.ui, Ui::Snap::Fill|Ui::Snap::NoPad, {});

        /** @todo some numeric-only validators, length restriction, once the UI
            lib is capable of that */
        _ui.metalness = Ui::Input{Ui::snap(_ui.ui, Ui::Snap::TopRight|Ui::Snap::Inside, root, WidgetSize), "0.5"};
        Ui::label(Ui::snap(_ui.ui, Ui::Snap::Left, _ui.metalness, WidgetSize), "Metalness", Text::Alignment::MiddleRight);

        _ui.roughness = Ui::Input{Ui::snap(_ui.ui, Ui::Snap::Bottom, _ui.metalness, WidgetSize), "0.25"};
        Ui::label(Ui::snap(_ui.ui, Ui::Snap::Left, _ui.roughness, WidgetSize), "Roughness", Text::Alignment::MiddleRight);

        _ui.f0 = Ui::Input{Ui::snap(_ui.ui, Ui::Snap::Bottom, _ui.roughness, WidgetSize), "0.25"};
        Ui::label(Ui::snap(_ui.ui, Ui::Snap::Left, _ui.f0, WidgetSize), "ƒ₀", Text::Alignment::MiddleRight);

        /** @todo enable the Apply button only if something changes once the UI
            library has a way to signal that, or at least has onKeyPress etc */
        Ui::NodeHandle apply = Ui::button(Ui::snap(_ui.ui, Ui::Snap::BottomRight|Ui::Snap::Inside, root, WidgetSize), "Apply", Ui::ButtonStyle::Primary);
        _ui.ui.eventLayer().onTapOrClick(apply, {*this, &AreaLightsExample::apply});

        Ui::NodeHandle reset = Ui::button(Ui::snap(_ui.ui, Ui::Snap::Top, apply, WidgetSize), "Reset", Ui::ButtonStyle::Danger);
        _ui.ui.eventLayer().onTapOrClick(reset, {*this, &AreaLightsExample::reset});
    }

    /* On Emscritpten we need to explicitly startTextInput() in order to get
       any text input events */
    /** @todo call this from the UI directly */
    #ifdef CORRADE_TARGET_EMSCRIPTEN
    startTextInput();
    #endif

    /* Apply the default values */
    apply();
}

void AreaLightsExample::apply() {
    _metalness = Math::clamp(std::stof(_ui.metalness.text()), 0.1f, 1.0f);
    _roughness = Math::clamp(std::stof(_ui.roughness.text()), 0.1f, 1.0f);
    _f0 = Math::clamp(std::stof(_ui.f0.text()), 0.1f, 1.0f);

    _areaLightShader.setMetalness(_metalness)
        .setRoughness(_roughness)
        .setF0(_f0);

    /* Set the clamped values back */
    _ui.metalness.setText(Utility::formatString("{:.5}", _metalness));
    _ui.roughness.setText(Utility::formatString("{:.5}", _roughness));
    _ui.f0.setText(Utility::formatString("{:.5}", _f0));
}

void AreaLightsExample::reset() {
    _ui.metalness.setText("0.5");
    _ui.roughness.setText("0.25");
    _ui.f0.setText("0.25");

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
            .setNormalMatrix(_transformation.normalMatrix())
            .setViewPosition(_view.invertedRigid().translation())
            .setLightQuad(quadPoints)
            .setBaseColor(_lightColor[i])
            .setLightIntensity(_lightIntensity[i])
            .setTwoSided(_lightTwoSided[i]);

        if(i == 0)
            GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

        _areaLightShader.draw(_plane);

        if(i == 0)
            GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    }
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    /* Draw light visualization, this time with depth test enabled for all.
       Draw twice for two-sided lights. */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    for(std::size_t i: {0, 1, 2}) {
        _flatShader
            .setColor(_lightColor[i]*_lightIntensity[i]*1.25f)
            .setTransformationProjectionMatrix(_projection*_view*_lightTransform[i])
            .draw(_plane);

        if(_lightTwoSided[i]) {
            _flatShader
                .setTransformationProjectionMatrix(_projection*_view*_lightTransform[i]*Matrix4::scaling(Vector3::xScale(-1.0f)))
                .draw(_plane);
        }
    }
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

    /* Draw the UI */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    _ui.ui.draw();
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::One);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    /* Redraw only if moving somewhere */
    swapBuffers();
    if(!_cameraDirection.isZero()) redraw();
}

void AreaLightsExample::pointerPressEvent(PointerEvent& event) {
    if(event.isPrimary() &&
       (event.pointer() & (Pointer::MouseLeft|Pointer::Finger)))
        _previousPointerPosition = event.position();

    if(!_ui.ui.pointerPressEvent(event))
        redraw();

    if(_ui.ui.state())
        redraw();
}

void AreaLightsExample::pointerReleaseEvent(PointerEvent& event) {
    if(!_ui.ui.pointerReleaseEvent(event))
        redraw();

    if(_ui.ui.state())
        redraw();
}

void AreaLightsExample::pointerMoveEvent(PointerMoveEvent& event) {
    if(_ui.ui.pointerMoveEvent(event)) {
        /* UI handles it */

    } else if(event.isPrimary() &&
              (event.pointers() & (Pointer::MouseLeft|Pointer::Finger))) {
        const Vector2 delta = 3.0f*
            (event.position() - _previousPointerPosition)/
            Vector2{GL::defaultFramebuffer.viewport().size()};
        _cameraRotation += delta;

        _previousPointerPosition = event.position();
        redraw();
    }

    if(_ui.ui.state())
        redraw();
}

void AreaLightsExample::keyPressEvent(KeyEvent& event) {
    /* If the UI accepts an input event, pass them only there  */
    if(_ui.ui.keyPressEvent(event)) {
        /* Redraw at the end */

    /* Movement */
    } else if(event.key() == Key::W) {
        _cameraDirection = -_view.inverted().backward()*0.01f;
    } else if(event.key() == Key::S) {
        _cameraDirection = +_view.inverted().backward()*0.01f;
    } else if(event.key() == Key::A) {
        _cameraDirection = +Math::cross(_view.inverted().backward(),
                                       Vector3::yAxis())*0.01f;
    } else if(event.key() == Key::D) {
        _cameraDirection = -Math::cross(_view.inverted().backward(),
                                        Vector3::yAxis())*0.01f;

    /* Increase/decrease roughness */
    } else if(event.key() == Key::R) {
        _roughness = Math::clamp(
            _roughness + 0.01f*(event.modifiers() & Modifier::Shift ? -1 : 1),
            0.1f, 1.0f);
        _areaLightShader.setRoughness(_roughness);
        _ui.roughness.setText(Utility::formatString("{:.5}", _roughness));

    /* Increase/decrease metalness */
    } else if(event.key() == Key::M) {
        _metalness = Math::clamp(
            _metalness + 0.01f*(event.modifiers() & Modifier::Shift ? -1 : 1),
            0.1f, 1.0f);
        _areaLightShader.setMetalness(_metalness);
        _ui.metalness.setText(Utility::formatString("{:.5}", _metalness));

    /* Increase/decrease f0 */
    } else if(event.key() == Key::F) {
        _f0 = Math::clamp(
            _f0 + 0.01f*(event.modifiers() & Modifier::Shift ? -1 : 1),
            0.1f, 1.0f);
        _areaLightShader.setF0(_f0);
        _ui.f0.setText(Utility::formatString("{:.5}", _f0));

    /* Reload shader */
    } else if(event.key() == Key::F5) {
        #ifdef CORRADE_IS_DEBUG_BUILD
        Utility::Resource::overrideGroup("arealights-data", "../src/arealights/resources.conf");
        _areaLightShader = AreaLightShader{};
        #endif

    } else return;

    redraw();
}

void AreaLightsExample::keyReleaseEvent(KeyEvent& event) {
    if(event.key() == Key::W || event.key() == Key::S ||
       event.key() == Key::A || event.key() == Key::D)
        _cameraDirection = {};
    else return;

    redraw();
}

void AreaLightsExample::textInputEvent(TextInputEvent& event) {
    _ui.ui.textInputEvent(event);

    if(_ui.ui.state())
        redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::AreaLightsExample)
