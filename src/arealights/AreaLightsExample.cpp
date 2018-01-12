/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
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

#include <Corrade/PluginManager/PluginManager.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/AbstractShaderProgram.h>
#include <Magnum/Buffer.h>
#include <Magnum/Context.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Extensions.h>
#include <Magnum/Mesh.h>
#include <Magnum/Renderer.h>
#include <Magnum/Shader.h>
#include <Magnum/Texture.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Version.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData3D.h>

#include "configure.h"

namespace Magnum { namespace Examples {

/* Class for the area light shader */
class AreaLightShader: public AbstractShaderProgram {
    public:
        explicit AreaLightShader() {
            MAGNUM_ASSERT_VERSION_SUPPORTED(Version::GL430);

            /* Load and compile shaders from compiled-in resource */
            Utility::Resource rs("arealights-data");

            Shader vert{Version::GL430, Shader::Type::Vertex};
            Shader frag{Version::GL430, Shader::Type::Fragment};

            vert.addSource(rs.get("AreaLights.vert"));
            frag.addSource(rs.get("AreaLights.frag"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(Shader::compile({vert, frag}));

            attachShaders({vert, frag});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            /* Get uniform locations */
            _transformationMatrixUniform = uniformLocation("u_transformationMatrix");
            _projectionMatrixUniform = uniformLocation("u_projectionMatrix");
            _viewMatrixUniform = uniformLocation("u_viewMatrix");
            _normalMatrixUniform = uniformLocation("u_normalMatrix");
            _viewPositionUniform = uniformLocation("u_viewPosition");

            _baseColorUniform = uniformLocation("u_baseColor");
            _metallicUniform = uniformLocation("u_metallic");
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

        AreaLightShader& setMetallic(Float v) {
            setUniform(_metallicUniform, v);
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

        AreaLightShader& bindTextures(Texture2D& ltcMat, Texture2D& ltcAmp) {
            Texture2D::bind(LtcMatTextureUnit, {&ltcMat, &ltcAmp});
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
            _metallicUniform,
            _roughnessUniform,
            _f0Uniform,
            _lightIntensityUniform,
            _twoSidedUniform,
            _lightQuadUniform;
};

using namespace Math::Literals;

class AreaLightsExample: public Platform::Application {
    public:
        explicit AreaLightsExample(const Arguments& arguments);

    private:
        void drawEvent() override;

        void mousePressEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;

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
        Buffer _vertices;
        Mesh _plane;

        /* Shaders */
        AreaLightShader _areaLightShader;
        Shaders::Flat3D _flatShader;

        /* Look Up Textures for arealights shader */
        Texture2D _ltcAmp;
        Texture2D _ltcMat;

        /* Camera and interaction */
        Matrix4 _transformation, _projection, _view;
        Vector2i _previousMousePosition;

        Vector3 _cameraPosition{0.0f, 1.0f, 6.0f};
        Vector3 _cameraDirection;
        Vector2 _cameraRotation;

        /* Material properties */
        Float _metallic = 0.5f;
        Float _roughness = 0.25f;
        Float _f0 = 0.5f; /* Specular reflection coefficient */
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

AreaLightsExample::AreaLightsExample(const Arguments& arguments):
    Platform::Application{arguments,
        Configuration{}.setTitle("Magnum Area Lights Example")
                       .setSampleCount(8)}
{
    /* Make it all DARK, eanble face culling so one-sided lights are properly
       visualized */
    Renderer::enable(Renderer::Feature::FaceCulling);
    Renderer::setClearColor(0x000000_rgbf);

    /* Setup the plane mesh, which will be used for both the floor and light
       visualization */
    _vertices.setData(LightVertices, BufferUsage::StaticDraw);
    _plane.setPrimitive(MeshPrimitive::TriangleFan)
        .addVertexBuffer(_vertices, 0, Shaders::Generic3D::Position{}, Shaders::Generic3D::Normal{})
        .setCount(Containers::arraySize(LightVertices));

    /* Setup project and floor plane tranformation matrix */
    _projection = Matrix4::perspectiveProjection(60.0_degf, 4.0f/3.0f, 0.1f, 50.0f);
    _transformation = Matrix4::rotationX(-90.0_degf)*Matrix4::scaling(Vector3{25.0f});

    /* Load LTC matrix and BRDF textures */
    PluginManager::Manager<Trade::AbstractImporter> manager{MAGNUM_PLUGINS_IMPORTER_DIR};
    std::unique_ptr<Trade::AbstractImporter> importer = manager.loadAndInstantiate("DdsImporter");
    if(!importer) std::exit(1);

    const Utility::Resource rs{"arealights-data"};
    if(!importer->openData(rs.getRaw("ltc_amp.dds")))
        std::exit(2);

    /* Set texture data and parameters */
    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    _ltcAmp.setWrapping(Sampler::Wrapping::ClampToEdge)
        .setMagnificationFilter(Sampler::Filter::Linear)
        .setMinificationFilter(Sampler::Filter::Linear)
        .setStorage(1, TextureFormat::RG32F, image->size())
        .setSubImage(0, {}, *image);

    if(!importer->openData(rs.getRaw("ltc_mat.dds")))
        std::exit(2);

    /* Set texture data and parameters */
    image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    _ltcMat.setWrapping(Sampler::Wrapping::ClampToEdge)
        .setMagnificationFilter(Sampler::Filter::Linear)
        .setMinificationFilter(Sampler::Filter::Linear)
        .setStorage(1, TextureFormat::RGBA32F, image->size())
        .setSubImage(0, {}, *image);

    _areaLightShader
        .bindTextures(_ltcMat, _ltcAmp)
        .setMetallic(_metallic)
        .setRoughness(_roughness)
        .setF0(_f0);
}

void AreaLightsExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color);

    /* Update view matrix */
    _cameraPosition += _cameraDirection;
    _view = Matrix4::rotationX(Rad{_cameraRotation.y()})*
            Matrix4::rotationY(Rad{_cameraRotation.x()})*
            Matrix4::translation(-_cameraPosition);

    /* Draw light on the floor. Cheat a bit and just add everything together.
       Will work as long as the background is black. */
    Renderer::enable(Renderer::Feature::Blending);
    Renderer::setBlendFunction(Renderer::BlendFunction::One, Renderer::BlendFunction::One);
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
        _plane.draw(_areaLightShader);
    }
    Renderer::disable(Renderer::Feature::Blending);

    /* Draw light visualization. Draw twice for two-sided lights. */
    for(std::size_t i: {0, 1, 2}) {
        _flatShader.setColor(_lightColor[i]*_lightIntensity[i]*0.75f)
            .setTransformationProjectionMatrix(_projection*_view*_lightTransform[i]);
        _plane.draw(_flatShader);

        if(_lightTwoSided[i]) {
            _flatShader.setTransformationProjectionMatrix(_projection*_view*_lightTransform[i]*Matrix4::scaling(Vector3::xScale(-1.0f)));
            _plane.draw(_flatShader);
        }
    }

    /* Redraw only if moving somewhere */
    swapBuffers();
    if(!_cameraDirection.isZero()) redraw();
}

void AreaLightsExample::mousePressEvent(MouseEvent& event) {
    if((event.button() == MouseEvent::Button::Left))
        _previousMousePosition = event.position();
    else return;

    redraw();
}

void AreaLightsExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector2 delta = 3.0f*
        Vector2{event.position() - _previousMousePosition}/Vector2{defaultFramebuffer.viewport().size()};
    _cameraRotation += delta;

    _previousMousePosition = event.position();
    redraw();
}

void AreaLightsExample::keyPressEvent(KeyEvent& event) {
    /* Movement */
    if(event.key() == KeyEvent::Key::W) {
        _cameraDirection = -_view.inverted().backward()*0.01f;
    } else if(event.key() == KeyEvent::Key::S) {
        _cameraDirection = _view.inverted().backward()*0.01f;
    } else if (event.key() == KeyEvent::Key::A) {
        _cameraDirection = Math::cross(_view.inverted().backward(), {0.0f, 1.0f, 0.0})*0.01f;
    } else if (event.key() == KeyEvent::Key::D) {
        _cameraDirection = -Math::cross(_view.inverted().backward(), { 0.0f, 1.0f, 0.0 })*0.01f;

    /* Increase/decrease roughness */
    } else if(event.key() == KeyEvent::Key::R) {
        _areaLightShader.setRoughness(_roughness = Math::clamp(
            _roughness + 0.01f*(event.modifiers() & KeyEvent::Modifier::Shift ? -1 : 1),
            0.1f, 1.0f));

    /* Increase/decrease metallic */
    } else if(event.key() == KeyEvent::Key::M) {
        _areaLightShader.setMetallic(_metallic = Math::clamp(
            _metallic + 0.01f*(event.modifiers() & KeyEvent::Modifier::Shift ? -1 : 1),
            0.1f, 1.0f));

    /* Increase/decrease f0 */
    } else if(event.key() == KeyEvent::Key::F) {
        _areaLightShader.setF0(_f0 = Math::clamp(
            _f0 + 0.01f*(event.modifiers() & KeyEvent::Modifier::Shift ? -1 : 1),
            0.1f, 1.0f));

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

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::AreaLightsExample)
