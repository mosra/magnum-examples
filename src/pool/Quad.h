#ifndef Quad_h
#define Quad_h

#include <array>
#include <Object.h>
#include <Mesh.h>
#include <Texture.h>

#include "PoolShader.h"

namespace Magnum {
    class Light;
}

class Quad: public Magnum::Object {
    public:
        Quad(const std::array<Magnum::Light*, PoolShader::LightCount>& lights, Object* parent = nullptr);

        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::Camera* camera);

    private:
        Magnum::Mesh mesh;
        std::array<Magnum::Light*, PoolShader::LightCount> lights;
        Magnum::Texture2D diffuse;
        PoolShader shader;
};

#endif
