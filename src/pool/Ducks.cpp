#include "Ducks.h"

#include <Math/Constants.h>
#include <MeshTools/Interleave.h>
#include <MeshTools/CompressIndices.h>
#include <SceneGraph/AbstractCamera.h>
#include <Trade/MeshData3D.h>
#include <Trade/MeshObjectData3D.h>
#include <Trade/ObjectData3D.h>
#include <Trade/PhongMaterialData.h>

namespace Magnum { namespace Examples {

Ducks* Ducks::tryCreate(Corrade::PluginManager::PluginManager<Trade::AbstractImporter>* manager, const Point3D& lightPosition, Object* parent, SceneGraph::DrawableGroup3D<>* drawables, SceneGraph::AnimableGroup3D<>* animables) {
    /* Load Collada importer */
    Trade::AbstractImporter* colladaImporter;
    if(manager->load("ColladaImporter") != Corrade::PluginManager::AbstractPluginManager::LoadOk || !(colladaImporter = manager->instance("ColladaImporter"))) {
        Error() << "Cannot load ColladaImporter plugin from" << manager->pluginDirectory();
        return nullptr;
    }

    /* Find duck object */
    if(!colladaImporter->open("duck.dae"))
        return nullptr;
    Trade::ObjectData3D* object = colladaImporter->object3D(0);
    if(object->instanceType() != Trade::ObjectData3D::InstanceType::Mesh) {
        Error() << "Cannot find duck in the file";
        return nullptr;
    }

    /* Get diffuse color */
    Trade::AbstractMaterialData* material = colladaImporter->material(static_cast<Trade::MeshObjectData3D*>(object)->material());
    if(material->type() != Trade::AbstractMaterialData::Type::Phong) {
        Error() << "The duck is not made of plastic.";
        return nullptr;
    }

    /* Get duck mesh data */
    Trade::MeshData3D* data = colladaImporter->mesh3D(object->instanceId());
    if(!data || !data->indices() || !data->positionArrayCount() || !data->normalArrayCount()) {
        Error() << "The duck is probably dead.";
        return nullptr;
    }

    Ducks* ducks = new Ducks(lightPosition, parent, animables);
    ducks->color = static_cast<Trade::PhongMaterialData*>(material)->diffuseColor();

    /* Prepare mesh */
    ducks->mesh.setPrimitive(data->primitive())
        ->addInterleavedVertexBuffer(&ducks->vertexBuffer, 0, Shaders::PhongShader::Position(), Shaders::PhongShader::Normal())
        ->setIndexBuffer(&ducks->indexBuffer);
    MeshTools::interleave(&ducks->mesh, &ducks->vertexBuffer, Buffer::Usage::StaticDraw, *data->positions(0), *data->normals(0));
    MeshTools::compressIndices(&ducks->mesh, &ducks->indexBuffer, Buffer::Usage::StaticDraw, *data->indices());

    /* Floating ducks */
    ducks->floatingDucks[0] = new Duck(ducks, drawables);
    ducks->floatingDucksTransformation[0] =
        Matrix4::translation({0.6f, 0.0f, 0.3f})*Matrix4::rotationY(deg(11.0f))*object->transformation();

    ducks->floatingDucks[1] = new Duck(ducks, drawables);
    ducks->floatingDucksTransformation[1] =
        Matrix4::translation({-0.4f, 0.0f, 0.52f})*Matrix4::rotationY(deg(-73.0f))*object->transformation();

    ducks->floatingDucks[2] = new Duck(ducks, drawables);
    ducks->floatingDucksTransformation[2] =
        Matrix4::translation({0.1f, 0.0f, -0.47f})*Matrix4::rotationY(deg(56.0f))*object->transformation();

    /* Dead ducks */
    (new Duck(ducks, drawables))
        ->setTransformation(object->transformation())
        ->rotate(deg(119.0f), Vector3(0.9f, 0.2f, 0.0f).normalized())
        ->translate({0.23f, 0.10f, -1.43f});
    (new Duck(ducks, drawables))
        ->setTransformation(object->transformation())
        ->rotate(deg(119.0f), Vector3(0.9f, 0.2f, 0.0f).normalized())
        ->rotateY(deg(97.0f))
        ->translate({-0.12f, 0.10f, -1.32f});

    return ducks;
}

Ducks::Ducks(const Point3D& lightPosition, Object3D* parent, Magnum::SceneGraph::AnimableGroup3D<>* animables): Object3D(parent), SceneGraph::Animable3D<>(this, 0.0f, animables), lightPosition(lightPosition), rotatedAxis(Vector3::xAxis()) {
    setState(SceneGraph::AnimationState::Running);
}

void Ducks::animationStep(GLfloat, GLfloat delta) {
    rotatedAxis = Matrix4::rotationY(deg(300.0f)*delta)*rotatedAxis;

    /* Fix floating-point drifting */
    rotatedAxis.xyz() = rotatedAxis.xyz().normalized();

    /* Ducks on waves */
    floatingDucks[0]->setTransformation(floatingDucksTransformation[0])
        ->rotate(deg(150.0f)*delta, rotatedAxis.xyz(), SceneGraph::TransformationType::Local);
    floatingDucks[1]->setTransformation(floatingDucksTransformation[1])
        ->rotate(deg(90.0f)*delta, -rotatedAxis.xyz(), SceneGraph::TransformationType::Local);
    floatingDucks[2]->setTransformation(floatingDucksTransformation[2])
        ->rotate(deg(60.0f)*delta, rotatedAxis.xyz(), SceneGraph::TransformationType::Local);
}

void Ducks::flip() {
    floatingDucksTransformation[0] = floatingDucksTransformation[0]*Matrix4::scaling(Vector3::zScale(-1.0f));
    floatingDucksTransformation[1] = floatingDucksTransformation[1]*Matrix4::scaling(Vector3::zScale(-1.0f));
    floatingDucksTransformation[2] = floatingDucksTransformation[2]*Matrix4::scaling(Vector3::zScale(-1.0f));
}

Ducks::Duck::Duck(Ducks* parent, SceneGraph::DrawableGroup3D<>* drawables): Object3D(parent), SceneGraph::Drawable3D<>(this, drawables), group(parent) {}

void Ducks::Duck::draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D<>* camera) {
    group->shader.setAmbientColor(group->color/2.0f)
        ->setDiffuseColor(group->color)
        ->setTransformationMatrix(transformationMatrix)
        ->setProjectionMatrix(camera->projectionMatrix())
        ->setLightPosition((camera->cameraMatrix()*group->lightPosition).xyz())
        ->use();

    group->mesh.draw();
}

}}
