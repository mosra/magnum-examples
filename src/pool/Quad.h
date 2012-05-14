#ifndef Quad_h
#define Quad_h

#include <array>
#include <PluginManager/PluginManager.h>
#include <Object.h>
#include <Mesh.h>
#include <Texture.h>
#include <Trade/AbstractImporter.h>

#include "PoolShader.h"

namespace Magnum {
    class Light;
}

class Quad: public Magnum::Object {
    public:
        Quad(Corrade::PluginManager::PluginManager<Magnum::Trade::AbstractImporter>* manager, const std::array<Magnum::Light*, PoolShader::LightCount>& lights, Object* parent = nullptr);

        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::Camera* camera);

    private:
        Magnum::Mesh mesh;
        std::array<Magnum::Light*, PoolShader::LightCount> lights;
        Magnum::Texture2D diffuse;
        Magnum::Texture2D specular;
        Magnum::Texture2D water;
        PoolShader shader;

        Magnum::Vector2 translation;
};

#endif
