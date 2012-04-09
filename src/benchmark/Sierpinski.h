#ifndef Sierpinski_h
#define Sierpinski_h

#include <Object.h>
#include <IndexedMesh.h>

#include "ColorShader.h"

class Sierpinski: public Magnum::Object {
    public:
        Sierpinski(Magnum::Object* parent = nullptr);

        void setIterations(size_t iterations, bool tipsify);

        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::Camera* camera);

        ColorShader* shader;

    private:
        Magnum::IndexedMesh mesh;
        Magnum::Buffer* buffer;
};

#endif
