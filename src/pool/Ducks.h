#ifndef Ducks_h
#define Ducks_h

#include <PluginManager/PluginManager.h>
#include <IndexedMesh.h>
#include <Object.h>
#include <Shaders/PhongShader.h>
#include <Trade/AbstractImporter.h>

namespace Magnum {
    class Light;
}

class Ducks: public Magnum::Object {
    public:
        static Ducks* tryCreate(Corrade::PluginManager::PluginManager<Magnum::Trade::AbstractImporter>* manager, Magnum::Light* light, Object* parent = nullptr);

        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::Camera* camera);

    private:
        class Duck: public Magnum::Object {
            public:
                inline Duck(Ducks* group): Object(group), group(group) {}

                void draw(const Magnum::Matrix4& transformationMatrix, Magnum::Camera* camera);

            private:
                Ducks* group;
        };

        inline Ducks(Magnum::Light* light, Object* parent = nullptr): Object(parent), light(light), rotatedAxis(1.0f, 0.0f, 0.0f) {}

        Magnum::IndexedMesh mesh;
        Magnum::Vector3 color;
        Magnum::Shaders::PhongShader shader;
        Magnum::Light* light;

        Duck* floatingDucks[3];
        Magnum::Matrix4 floatingDucksTransformation[3];
        Magnum::Vector4 rotatedAxis;
};

#endif
