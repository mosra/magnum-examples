#include "Quad.h"

#include <Camera.h>
#include <Image.h>
#include <Light.h>

using namespace std;
using namespace Corrade::PluginManager;
using namespace Corrade::Utility;
using namespace Magnum;

Quad::Quad(PluginManager<Trade::AbstractImporter>* manager, const array<Light*, PoolShader::LightCount>& lights, Object* parent): Object(parent), mesh(Mesh::Primitive::TriangleStrip, 4), lights(lights), diffuse(0), specular(1), water(2), translation(0.0f, 0.1f) {
    static const unsigned int textureSize = 32;

    /* Generate radial gradient */
    Math::Vector3<GLubyte> color1(160, 160, 255);
    Math::Vector3<GLubyte> color2(200, 200, 255);
    Math::Vector3<GLubyte>* radialGradient = new Math::Vector3<GLubyte>[Math::pow<2>(textureSize)]();
    for(unsigned int x = 1; x != textureSize-1; ++x) {
        for(unsigned int y = 1; y != textureSize-1; ++y) {
            float t = (Vector2(x, y) - Vector2(0.0f, 0.0f)).length()/(textureSize-1);
            if(t > 1.0f) t = 1.0f;

            radialGradient[y*textureSize+x] = color1*(1-t) + color2*t;
        }
    }
    Image2D radialGradientImage({32, 32}, Image2D::Components::RGB, Image2D::ComponentType::UnsignedByte, radialGradient);

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
    Image2D specularMapImage({32, 32}, Image2D::Components::Red, Image2D::ComponentType::UnsignedByte, specularMap);

    /* Load TGA importer plugin */
    Trade::AbstractImporter* importer;
    if(manager->load("TgaImporter") != AbstractPluginManager::LoadOk || !(importer = manager->instance("TgaImporter"))) {
        Error() << "Cannot load TgaImporter plugin from" << manager->pluginDirectory();
        exit(1);
    }

    /* Load water texture */
    Resource rs("pool");
    std::istringstream in(rs.get("normal.tga"));
    if(!importer->open(in) || !importer->image2DCount()) {
        Error() << "Cannot load water texture";
        exit(2);
    }

    diffuse.setData(0, Texture2D::Components::RGBA|Texture2D::ComponentType::NormalizedUnsignedByte, &radialGradientImage);
    diffuse.setWrapping(Math::Vector2<Texture2D::Wrapping>(Texture2D::Wrapping::MirroredRepeat));
    diffuse.setMinificationFilter(Texture2D::Filter::LinearInterpolation, Texture2D::Mipmap::LinearInterpolation);
    diffuse.setMagnificationFilter(Texture2D::Filter::LinearInterpolation);
    diffuse.generateMipmap();
    diffuse.setMaxAnisotropy(Texture2D::maxSupportedAnisotropy());

    specular.setData(0, Texture2D::Components::Red|Texture2D::ComponentType::NormalizedUnsignedByte, &specularMapImage);
    specular.setWrapping(Math::Vector2<Texture2D::Wrapping>(Texture2D::Wrapping::MirroredRepeat));
    specular.setMinificationFilter(Texture2D::Filter::LinearInterpolation, Texture2D::Mipmap::LinearInterpolation);
    specular.setMagnificationFilter(Texture2D::Filter::LinearInterpolation);
    specular.generateMipmap();
    specular.setMaxAnisotropy(Texture2D::maxSupportedAnisotropy());

    water.setData(0, Texture2D::Components::RGBA|Texture2D::ComponentType::NormalizedUnsignedByte, importer->image2D(0));
    water.setWrapping(Math::Vector2<Texture2D::Wrapping>(Texture2D::Wrapping::MirroredRepeat));
    water.setMinificationFilter(Texture2D::Filter::LinearInterpolation, Texture2D::Mipmap::LinearInterpolation);
    water.setMagnificationFilter(Texture2D::Filter::LinearInterpolation);
    water.generateMipmap();
    water.setMaxAnisotropy(Texture2D::maxSupportedAnisotropy());
}

void Quad::draw(const Matrix4& transformationMatrix, Camera* camera) {
    diffuse.bind();
    water.bind();
    shader.use();
    shader.setTransformationMatrixUniform(transformationMatrix);
    shader.setNormalMatrixUniform(transformationMatrix.rotation());
    shader.setProjectionMatrixUniform(camera->projectionMatrix());
    shader.setCameraDirectionUniform(-(camera->transformation()*Vector4()).xyz());
    shader.setDiffuseTextureUniform(&diffuse);
    shader.setSpecularTextureUniform(&specular);
    shader.setWaterTextureUniform(&water);

    translation = (Matrix4::rotation(deg(1.0f), Vector3::zAxis())*Vector4(translation.x(), translation.y(), 0.0f)).xyz().xy();

    shader.setWaterTextureTranslationUniform(translation);

    for(size_t i = 0; i != PoolShader::LightCount; ++i)
        shader.setLightPositionUniform(i, (camera->cameraMatrix()*lights[i]->position()).xyz());

    mesh.draw();
}
