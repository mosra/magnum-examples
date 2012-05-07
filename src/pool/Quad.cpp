#include "Quad.h"

#include <Camera.h>
#include <Image.h>

using namespace Magnum;

Quad::Quad(Object* parent): Object(parent), mesh(Mesh::Primitive::TriangleStrip, 4) {
    /* Generate radial gradient */
    static const unsigned int size = 32;

    Math::Vector3<GLubyte> color1(160, 160, 255);
    Math::Vector3<GLubyte> color2(200, 200, 255);

    Math::Vector3<GLubyte>* radialGradient = new Math::Vector3<GLubyte>[Math::pow<2>(size)]();

    for(unsigned int x = 1; x != size-1; ++x) {
        for(unsigned int y = 1; y != size-1; ++y) {
            float t = (Vector2(x, y) - Vector2(0.0f, 0.0f)).length()/(size-1);
            if(t > 1.0f) t = 1.0f;

            radialGradient[y*size+x] = color1*(1-t) + color2*t;
        }
    }

    Image2D radialGradientImage({32, 32}, Image2D::Components::RGB, Image2D::ComponentType::UnsignedByte, radialGradient);
    diffuse.setData(0, Texture2D::Components::RGBA|Texture2D::ComponentType::NormalizedUnsignedByte, &radialGradientImage);
    diffuse.setWrapping(Math::Vector2<Texture2D::Wrapping>(Texture2D::Wrapping::MirroredRepeat));
    diffuse.setMinificationFilter(Texture2D::Filter::LinearInterpolation, Texture2D::Mipmap::LinearInterpolation);
    diffuse.setMagnificationFilter(Texture2D::Filter::LinearInterpolation);
    diffuse.generateMipmap();
    diffuse.setMaxAnisotropy(Texture2D::maxSupportedAnisotropy());
}

void Quad::draw(const Matrix4& transformationMatrix, Camera* camera) {
    diffuse.bind();
    shader.use();
    shader.setTransformationMatrixUniform(transformationMatrix);
    shader.setProjectionMatrixUniform(camera->projectionMatrix());
    shader.setDiffuseTextureUniform(&diffuse);
    mesh.draw();
}
