#include "Ducks.h"

#include <MeshTools/Interleave.h>
#include <MeshTools/CompressIndices.h>
#include <Trade/MeshData.h>
#include <Trade/MeshObjectData.h>
#include <Trade/PhongMaterialData.h>
#include <Camera.h>
#include <Light.h>

using namespace Corrade::Utility;
using namespace Corrade::PluginManager;
using namespace Magnum;
using namespace Magnum::Shaders;

Ducks* Ducks::tryCreate(PluginManager<Trade::AbstractImporter>* manager, Light* light, Object* parent) {
    /* Load Collada importer */
    Trade::AbstractImporter* colladaImporter;
    if(manager->load("ColladaImporter") != AbstractPluginManager::LoadOk || !(colladaImporter = manager->instance("ColladaImporter"))) {
        Error() << "Cannot load ColladaImporter plugin from" << manager->pluginDirectory();
        return nullptr;
    }

    /* Find duck object */
    if(!colladaImporter->open("duck.dae"))
        return nullptr;
    Trade::ObjectData* object = colladaImporter->object(0);
    if(object->instanceType() != Trade::ObjectData::InstanceType::Mesh) {
        Error() << "Cannot find duck in the file";
        return nullptr;
    }

    /* Get diffuse color */
    Trade::AbstractMaterialData* material = colladaImporter->material(static_cast<Trade::MeshObjectData*>(object)->material());
    if(material->type() != Trade::AbstractMaterialData::Type::Phong) {
        Error() << "The duck is not made of plastic.";
        return nullptr;
    }

    /* Get duck mesh data */
    Trade::MeshData* data = colladaImporter->mesh(object->instanceId());
    if(!data || !data->indices() || !data->vertexArrayCount() || !data->normalArrayCount()) {
        Error() << "The duck is probably dead.";
        return nullptr;
    }

    Ducks* ducks = new Ducks(light, parent);
    ducks->color = static_cast<Trade::PhongMaterialData*>(material)->diffuseColor();

    /* Interleave mesh data */
    Buffer* buffer = ducks->mesh.addBuffer(true);
    ducks->mesh.bindAttribute<PhongShader::Vertex>(buffer);
    ducks->mesh.bindAttribute<PhongShader::Normal>(buffer);
    MeshTools::interleave(&ducks->mesh, buffer, Buffer::Usage::StaticDraw, *data->vertices(0), *data->normals(0));

    /* Compress indices */
    MeshTools::compressIndices(&ducks->mesh, Buffer::Usage::StaticDraw, *data->indices());

    /* Floating ducks */
    ducks->floatingDucks[0] = new Duck(ducks);
    ducks->floatingDucksTransformation[0] =
        Matrix4::translation({0.6f, 0.0f, 0.3f})*Matrix4::rotation(deg(11.0f), Vector3::yAxis())*object->transformation();

    ducks->floatingDucks[1] = new Duck(ducks);
    ducks->floatingDucksTransformation[1] =
        Matrix4::translation({-0.4f, 0.0f, 0.52f})*Matrix4::rotation(deg(-73.0f), Vector3::yAxis())*object->transformation();

    ducks->floatingDucks[2] = new Duck(ducks);
    ducks->floatingDucksTransformation[2] =
        Matrix4::translation({0.1f, 0.0f, -0.47f})*Matrix4::rotation(deg(56.0f), Vector3::yAxis())*object->transformation();

    /* Dead ducks */
    (new Duck(ducks))->setTransformation(object->transformation())
        ->rotate(deg(119.0f), {0.9f, 0.2f, 0.0f})->translate({0.23f, 0.10f, -1.43f});
    (new Duck(ducks))->setTransformation(object->transformation())
        ->rotate(deg(119.0f), {0.9f, 0.2f, 0.0f})->rotate(deg(97.0f), Vector3::yAxis())->translate({-0.12f, 0.10f, -1.32f});

    return ducks;
}

void Ducks::draw(const Matrix4& transformationMatrix, Camera* camera) {
    rotatedAxis = Matrix4::rotation(deg(5.0f), Vector3::yAxis())*rotatedAxis;

    /* Ducks on waves */
    floatingDucks[0]->setTransformation(floatingDucksTransformation[0])
        ->rotate(deg(5.0f), rotatedAxis.xyz(), Object::Transformation::Local);
    floatingDucks[1]->setTransformation(floatingDucksTransformation[1])
        ->rotate(deg(3.0f), -rotatedAxis.xyz(), Object::Transformation::Local);
    floatingDucks[2]->setTransformation(floatingDucksTransformation[2])
        ->rotate(deg(2.0f), rotatedAxis.xyz(), Object::Transformation::Local);
}

void Ducks::Duck::draw(const Matrix4& transformationMatrix, Camera* camera) {
    group->shader.use();
    group->shader.setAmbientColorUniform(group->color/2.0f);
    group->shader.setDiffuseColorUniform(group->color);
    group->shader.setTransformationMatrixUniform(transformationMatrix);
    group->shader.setProjectionMatrixUniform(camera->projectionMatrix());
    group->shader.setLightUniform((camera->cameraMatrix()*group->light->position()).xyz());

    group->mesh.draw();
}
