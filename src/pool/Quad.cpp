#include "Quad.h"

#include <sstream>
#include <Math/Constants.h>
#include <Context.h>
#include <Extensions.h>
#include <Image.h>
#include <Swizzle.h>
#include <SceneGraph/AbstractCamera.h>
#include <Trade/ImageData.h>

namespace Magnum { namespace Examples {

Quad::Quad(Corrade::PluginManager::PluginManager<Trade::AbstractImporter>* manager, const std::array<Point3D, PoolShader::LightCount>& lights, Object3D* parent, SceneGraph::DrawableGroup3D<>* drawables, SceneGraph::AnimableGroup3D<>* animables): Object3D(parent), SceneGraph::Drawable3D<>(this, drawables), SceneGraph::Animable3D<>(this, 0.0f, animables), lights(lights), translation(0.0f, 0.1f) {
    MAGNUM_ASSERT_EXTENSION_SUPPORTED(Extensions::GL::ARB::texture_rg);
    MAGNUM_ASSERT_EXTENSION_SUPPORTED(Extensions::GL::ARB::texture_storage);
    MAGNUM_ASSERT_EXTENSION_SUPPORTED(Extensions::GL::EXT::texture_filter_anisotropic);

    static const unsigned int textureSize = 32;

    /* Generate radial gradient */
    Color3<GLubyte> color1(160, 160, 255);
    Color3<GLubyte> color2(200, 200, 255);
    Color3<GLubyte>* radialGradient = new Color3<GLubyte>[Math::pow<2>(textureSize)]();
    for(unsigned int x = 1; x != textureSize-1; ++x) {
        for(unsigned int y = 1; y != textureSize-1; ++y) {
            float t = (Vector2(x, y) - Vector2(0.0f, 0.0f)).length()/(textureSize-1);
            if(t > 1.0f) t = 1.0f;

            radialGradient[y*textureSize+x] = Color3<GLubyte>::lerp(color1, color2, t);
        }
    }
    Image2D radialGradientImage({32, 32}, Image2D::Format::RGB, Image2D::Type::UnsignedByte, radialGradient);

    /* Generate specular map */
    GLubyte* specularMap = new GLubyte[Math::pow<2>(textureSize)]();
    for(unsigned int x = 0; x != textureSize; ++x) {
        for(unsigned int y = 0; y != textureSize; ++y) {
            /* Between tiles */
            if(!(x%textureSize) || !(y%textureSize))
                specularMap[y*textureSize+x] = 48;

            /* Tile inside */
            else if(x > 4 && x < 28 && y > 4 && y < 28)
                specularMap[y*textureSize+x] = 128;

            /* Tile border */
            else specularMap[y*textureSize+x] = 192;
        }
    }
    Image2D specularMapImage({32, 32}, Image2D::Format::Red, Image2D::Type::UnsignedByte, specularMap);

    /* Load TGA importer plugin */
    Trade::AbstractImporter* importer;
    if(manager->load("TgaImporter") != Corrade::PluginManager::AbstractPluginManager::LoadOk || !(importer = manager->instance("TgaImporter"))) {
        Error() << "Cannot load TgaImporter plugin from" << manager->pluginDirectory();
        std::exit(1);
    }

    /* Load water texture */
    Corrade::Utility::Resource rs("pool");
    std::istringstream in(rs.get("normal.tga"));
    if(!importer->open(in) || !importer->image2DCount()) {
        Error() << "Cannot load water texture";
        std::exit(2);
    }

    Trade::ImageData2D* waterImage = importer->image2D(0);

    diffuse.setWrapping(Texture2D::Wrapping::MirroredRepeat)
        ->setMinificationFilter(Texture2D::Filter::LinearInterpolation, Texture2D::Mipmap::LinearInterpolation)
        ->setMagnificationFilter(Texture2D::Filter::LinearInterpolation)
        ->setMaxAnisotropy(Texture2D::maxSupportedAnisotropy())
        ->setStorage(Math::log2(radialGradientImage.size().min())-1, Texture2D::InternalFormat::RGBA8, radialGradientImage.size())
        ->setSubImage(0, {}, &radialGradientImage)
        ->generateMipmap();

    specular.setWrapping(Texture2D::Wrapping::MirroredRepeat)
        ->setMinificationFilter(Texture2D::Filter::LinearInterpolation, Texture2D::Mipmap::LinearInterpolation)
        ->setMagnificationFilter(Texture2D::Filter::LinearInterpolation)
        ->setMaxAnisotropy(Texture2D::maxSupportedAnisotropy())
        ->setStorage(Math::log2(specularMapImage.size().min())-1, Texture2D::InternalFormat::R8, specularMapImage.size())
        ->setSubImage(0, {}, &specularMapImage)
        ->generateMipmap();

    water.setWrapping(Texture2D::Wrapping::MirroredRepeat)
        ->setMinificationFilter(Texture2D::Filter::LinearInterpolation, Texture2D::Mipmap::LinearInterpolation)
        ->setMagnificationFilter(Texture2D::Filter::LinearInterpolation)
        ->setMaxAnisotropy(Texture2D::maxSupportedAnisotropy())
        ->setStorage(Math::log2(waterImage->size().min())-1, Texture2D::InternalFormat::RGBA8, waterImage->size())
        ->setSubImage(0, {}, waterImage)
        ->generateMipmap();

    mesh.setPrimitive(Mesh::Primitive::TriangleStrip)
        ->setVertexCount(4);

    setState(SceneGraph::AnimationState::Running);
}

void Quad::animationStep(GLfloat, GLfloat delta) {
    translation = Matrix3::rotation(deg(15.0f)*delta)*translation;
}

void Quad::draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D<>* camera) {
    shader.setTransformationMatrix(transformationMatrix)
        ->setNormalMatrix(transformationMatrix.rotation())
        ->setProjectionMatrix(camera->projectionMatrix())
        ->setCameraDirection(-camera->object()->transformationMatrix().translation())
        ->setWaterTextureTranslation(translation.xy())
        ->use();

    for(size_t i = 0; i != PoolShader::LightCount; ++i)
        shader.setLightPosition(i, (camera->cameraMatrix()*lights[i]).xyz());

    diffuse.bind(PoolShader::DiffuseTextureLayer);
    specular.bind(PoolShader::SpecularTextureLayer);
    water.bind(PoolShader::WaterTextureLayer);

    mesh.draw();
}

}}
