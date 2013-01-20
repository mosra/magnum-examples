#ifndef Magnum_Examples_Ducks_h
#define Magnum_Examples_Ducks_h

#include <PluginManager/PluginManager.h>
#include <Buffer.h>
#include <Mesh.h>
#include <SceneGraph/Animable.h>
#include <SceneGraph/Drawable.h>
#include <SceneGraph/MatrixTransformation3D.h>
#include <SceneGraph/Object.h>
#include <Shaders/PhongShader.h>
#include <Trade/AbstractImporter.h>

#include "Types.h"

namespace Magnum { namespace Examples {

class Ducks: public Object3D, SceneGraph::Animable3D<> {
    public:
        static Ducks* tryCreate(Corrade::PluginManager::PluginManager<Magnum::Trade::AbstractImporter>* manager, const Point3D& lightPosition, Object3D* parent, SceneGraph::DrawableGroup3D<>* drawables, SceneGraph::AnimableGroup3D<>* animables);

        void flip();

    protected:
        void animationStep(GLfloat time, GLfloat delta) override;

    private:
        class Duck: public Object3D, SceneGraph::Drawable3D<> {
            public:
                Duck(Ducks* group, SceneGraph::DrawableGroup3D<>* drawables);

            protected:
                void draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D<>* camera) override;

            private:
                Ducks* group;
        };

        Ducks(const Point3D& lightPosition, Object3D* parent, SceneGraph::AnimableGroup3D<>* animables);

        Mesh mesh;
        Buffer vertexBuffer, indexBuffer;
        Vector3 color;
        Point3D lightPosition;
        Shaders::PhongShader shader;

        Duck* floatingDucks[3];
        Matrix4 floatingDucksTransformation[3];
        Point3D rotatedAxis;
};

}}

#endif
