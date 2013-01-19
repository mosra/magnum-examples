#ifndef Magnum_Examples_Quad_h
#define Magnum_Examples_Quad_h

#include <array>
#include <PluginManager/PluginManager.h>
#include <Mesh.h>
#include <Texture.h>
#include <SceneGraph/Animable.h>
#include <SceneGraph/Drawable.h>
#include <SceneGraph/MatrixTransformation3D.h>
#include <SceneGraph/Object.h>
#include <Trade/AbstractImporter.h>

#include "PoolShader.h"
#include "Types.h"

namespace Magnum { namespace Examples {

class Quad: public Object3D, SceneGraph::Drawable3D<>, SceneGraph::Animable3D<> {
    public:
        Quad(Corrade::PluginManager::PluginManager<Trade::AbstractImporter>* manager, const std::array<Point3D, PoolShader::LightCount>& lights, Object* parent, SceneGraph::DrawableGroup3D<>* drawables, SceneGraph::AnimableGroup3D<>* animables);

    protected:
        void draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D<>* camera) override;
        void animationStep(GLfloat time, GLfloat delta) override;

    private:
        Mesh mesh;
        std::array<Point3D, PoolShader::LightCount> lights;
        Texture2D diffuse, specular, water;
        PoolShader shader;

        Point2D translation;
};

}}

#endif
