/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
            Vladimír Vondruš <mosra@centrum.cz>
        2017 — Jonathan Hale <squareys@googlemail.com>

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
#include <Magnum/Renderer.h>
#include <Magnum/Shader.h>
#include <Magnum/Texture.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Version.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Plane.h>
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

        AreaLightShader& setViewPosition(const Vector4& pos) {
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

        AreaLightShader& setLightQuad(Containers::ArrayView<Vector4> quadPoints) {
            setUniform(_lightQuadUniform, quadPoints);
            return *this;
        }

        /* LTC lookup textures */

        AreaLightShader& bindLtcAmpTexture(Texture2D& ltcAmp) {
            ltcAmp.bind(1);
            return *this;
        }

        AreaLightShader& bindLtcMatTexture(Texture2D& ltcMat) {
            ltcMat.bind(0);
            return *this;
        }

    private:
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

        AreaLightShader _shader;
        Matrix4 _lightTransform;

        /* Floor plane mesh data */
        Buffer _vertexBuffer;
        Mesh _floorPlane;

        /* For light visualization */
        Buffer _lightVertexBuffer;
        Mesh _lightPlane;
        Shaders::Flat3D _flatShader;

        /* Look Up Textures for arealights shader */
        Texture2D _ltcAmp;
        Texture2D _ltcMat;

        /* Camera and interaction */
        Matrix4 _transformation, _projection, _view;
        Vector2i _previousMousePosition;

        Vector3 _cameraPosition{1.0f, -5.0f, -6.0f};
        Vector3 _cameraDirection;
        Vector2 _cameraRotation{0.4f, 0.4f};

        /* Material properties */
        Float _metallic = 0.5f;
        Float _roughness = 0.25f;
        Float _f0 = 0.5f; /* Specular reflection coefficient */
};

AreaLightsExample::AreaLightsExample(const Arguments& arguments):
    Platform::Application{arguments,
        Configuration{}.setTitle("Magnum Area Lights Example")
                       .setSampleCount(8)
                       .setVersion(Version::GL430)}
{
    Renderer::enable(Renderer::Feature::DepthTest);
    Renderer::disable(Renderer::Feature::FaceCulling);

    /* Setup floor plane mesh */
    const Trade::MeshData3D plane = Primitives::Plane::solid();
    _vertexBuffer.setData(MeshTools::interleave(plane.positions(0), plane.normals(0)), BufferUsage::StaticDraw);
    _floorPlane.setPrimitive(plane.primitive())
        .addVertexBuffer(_vertexBuffer, 0, Shaders::Generic3D::Position{}, Shaders::Generic3D::Normal{})
        .setCount(plane.positions(0).size());

    /* Setup project and floor plane tranformation matrix */
    const Float aspectRatio = Vector2{defaultFramebuffer.viewport().size()}.aspectRatio();
    _projection = Matrix4::perspectiveProjection(60.0_degf, aspectRatio, 0.01f, 100.0f);

    _transformation = Matrix4::rotationX(-90.0_degf)*Matrix4::scaling(Vector3{25.0f});

    /* Setup light transform and buffers */
    const Vector2 scale{3.0f, 3.0f};
    const Vector3 lightPosition{3.0f, 3.0f, -3.0f};
    _lightTransform = Matrix4::translation(lightPosition)*
                  Matrix4::rotationX(0.0_degf)*
                  Matrix4::rotationY(-45.0_degf)*
                  Matrix4::rotationZ(0.0_degf)*
                  Matrix4::scaling(Vector3(0.5f*scale, 1.0f));

    constexpr Vector3 lightVertices[] = {
        {-1.0f,  1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        {-1.0f, -1.0f, 0.0f}
    };

    Vector4 quadPoints[4];
    for(int i : {0, 1, 2, 3}) {
        quadPoints[i] = _lightTransform*Vector4{lightVertices[i], 1.0f};
    }

    /* Setup mesh for light visualization */
    _lightVertexBuffer.setData(lightVertices, BufferUsage::StaticDraw);
    _lightPlane
        .setPrimitive(MeshPrimitive::TriangleFan)
        .addVertexBuffer(_lightVertexBuffer, 0, Shaders::Flat3D::Position{})
        .setCount(4);

    /* Load LTC matrix and BRDF textures */
    PluginManager::Manager<Trade::AbstractImporter> manager{MAGNUM_PLUGINS_IMPORTER_DIR};
    std::unique_ptr<Trade::AbstractImporter> importer = manager.loadAndInstantiate("DdsImporter");
    if(!importer) std::exit(1);

    const Utility::Resource rs{"arealights-data"};
    if(!importer->openData(rs.getRaw("ltc_amp.dds")))
        std::exit(2);

    /* Set texture data and parameters */
    std::optional<Trade::ImageData2D> image = importer->image2D(0);
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

    _shader.bindLtcAmpTexture(_ltcAmp)
        .bindLtcMatTexture(_ltcMat)
        .setBaseColor(0xcccccc_rgbf)
        .setMetallic(_metallic)
        .setRoughness(_roughness)
        .setF0(_f0)
        .setLightIntensity(1.0f)
        .setTwoSided(true)
        .setLightQuad({quadPoints, 4});
}

void AreaLightsExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);

    /* Update view matrix */
    _cameraPosition += _cameraDirection;
    _view = Matrix4::rotationX(Rad{_cameraRotation.y()})*
            Matrix4::rotationY(Rad{_cameraRotation.x()})*
            Matrix4::translation(_cameraPosition);

    /* Draw floor plane */
    _shader.setTransformationMatrix(_transformation)
           .setProjectionMatrix(_projection)
           .setViewMatrix(_view)
           .setNormalMatrix(_transformation.rotationScaling())
        .setViewPosition(Vector4{ _view.inverted().transformPoint({}), 1.0f });
    _floorPlane.draw(_shader);

    /* Draw light */
    _flatShader.setTransformationProjectionMatrix(_projection*_view*_lightTransform)
        .setColor(Color3{1.0f, 1.0f, 1.0f});
    _lightPlane.draw(_flatShader);

    swapBuffers();
    redraw();
}

void AreaLightsExample::mousePressEvent(MouseEvent& event) {
    if((event.button() == MouseEvent::Button::Left)) {
        _previousMousePosition = event.position();
    }
}

void AreaLightsExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    // TODO: size is 0,0 ?
    const Vector2 delta = 3.0f*
        Vector2{event.position() - _previousMousePosition}/Vector2{defaultFramebuffer.viewport().size()};
    _cameraRotation += delta;

    _previousMousePosition = event.position();
    event.setAccepted();
}

void AreaLightsExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::W) {
        _cameraDirection = _view.inverted().backward();
    } else if(event.key() == KeyEvent::Key::S) {
        _cameraDirection = -_view.inverted().backward();
    } else if (event.key() == KeyEvent::Key::A) {
        _cameraDirection = -Math::cross(_view.inverted().backward(), {0.0f, 1.0f, 0.0});
    } else if (event.key() == KeyEvent::Key::D) {
        _cameraDirection = Math::cross(_view.inverted().backward(), { 0.0f, 1.0f, 0.0 });
    } else if(event.key() == KeyEvent::Key::R) {
        /* Increase/decrease roughness */
        if(event.modifiers() & KeyEvent::Modifier::Shift) {
            _roughness = Math::max(0.1f, _roughness - 0.1f);
        } else {
            _roughness = Math::min(1.0f, _roughness + 0.1f);
        }
        _shader.setRoughness(_roughness);
    } else if(event.key() == KeyEvent::Key::M) {
        /* Increase/decrease metallic */
        if(event.modifiers() & KeyEvent::Modifier::Shift) {
            _metallic = Math::max(0.1f, _metallic - 0.1f);
        } else {
            _metallic = Math::min(1.0f, _metallic + 0.1f);
        }
        _shader.setMetallic(_metallic);
    } else if(event.key() == KeyEvent::Key::F) {
        /* Increase/decrease f0 */
        if(event.modifiers() & KeyEvent::Modifier::Shift) {
            _f0 = Math::max(0.1f, _f0 - 0.1f);
        } else {
            _f0 = Math::min(1.0f, _f0 + 0.1f);
        }
        _shader.setF0(_f0);
    } else if(event.key() == KeyEvent::Key::Esc) {
        exit();
    }

    #ifdef CORRADE_IS_DEBUG_BUILD
    if(event.key() == KeyEvent::Key::F5) {
        /* Reload shader */
        Utility::Resource::overrideGroup("arealights-data", "../src/arealights/resources.conf");
        _shader = AreaLightShader{};
    }
    #endif
}

void AreaLightsExample::keyReleaseEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::W || event.key() == KeyEvent::Key::S ||
       event.key() == KeyEvent::Key::A || event.key() == KeyEvent::Key::D) {
        _cameraDirection = {};
    }
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::AreaLightsExample)
