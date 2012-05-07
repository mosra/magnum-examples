#ifndef Quad_h
#define Quad_h

#include <Object.h>
#include <Mesh.h>
#include <Texture.h>

#include "PoolShader.h"

class Quad: public Magnum::Object {
    public:
        Quad(Object* parent = nullptr);

        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::Camera* camera);

    private:
        Magnum::Mesh mesh;
        Magnum::Texture2D diffuse;
        PoolShader shader;
};

#endif
