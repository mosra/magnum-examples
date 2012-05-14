/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include "JavaViewer.h"

#include <memory>
#include <IndexedMesh.h>
#include <MeshTools/Interleave.h>
#include <MeshTools/CompressIndices.h>
#include <Trade/MeshData.h>
#include <Trade/MeshObjectData.h>
#include <Trade/SceneData.h>

#include "configure.h"
#include "ViewedObject.h"

using namespace std;
using namespace Corrade::Utility;
using namespace Corrade::PluginManager;
using namespace Magnum::Trade;

namespace Magnum { namespace Examples {

JavaViewer::JavaViewer(): _camera(&scene), manager(PLUGIN_IMPORTER_DIR), o(nullptr) {
    _camera.setPerspective(deg(35.0f), 0.001f, 100);
    _camera.translate(Vector3::zAxis(5));
    _camera.setClearColor(Vector3(0.3f));
    Camera::setFeature(Camera::Feature::DepthTest, true);
    Camera::setFeature(Camera::Feature::FaceCulling, true);

    /* Set default light positions and colors */
    _shader.use();
    _shader.setLightColorUniform(0, Vector3(1.0));
    _shader.setLightColorUniform(1, Vector3(0.2, 0.0, 0.0));
    _shader.setLightColorUniform(2, Vector3(1.0));

    /* Load ColladaImporter plugin */
    if(manager.load("ColladaImporter") != AbstractPluginManager::LoadOk) {
        Error() << "Could not load ColladaImporter plugin";
        exit(1);
    }
}

JavaViewer::~JavaViewer() {
    manager.unload("ColladaImporter");

    close();
}

bool JavaViewer::openCollada(const std::string& filename) {
    /* Delete previous imported file */
    if(o) {
        delete o;
        for(auto i: meshes)
            delete i.second;
        meshes.clear();
    }

    /* Instance ColladaImporter plugin */
    unique_ptr<AbstractImporter> colladaImporter(manager.instance("ColladaImporter"));
    if(!colladaImporter) {
        Error() << "Could not instance ColladaImporter plugin";
        return false;
    }

    /* Load file */
    if(!colladaImporter->open(filename)) return false;

    if(colladaImporter->sceneCount() == 0) {
        Warning() << "The file has no scene";
        return false;
    }

    /* Map with materials */
    unordered_map<size_t, PhongMaterialData*> materials;

    /* Default object, parent of all (for manipulation) */
    o = new Object(&scene);

    /* Load the scene */
    SceneData* scene = colladaImporter->scene(colladaImporter->defaultScene());

    /* Add all children */
    for(size_t objectId: scene->children())
        addObject(colladaImporter.get(), o, materials, objectId);

    /* Delete materials, as they are now unused */
    for(auto i: materials) delete i.second;

    colladaImporter->close();
    delete colladaImporter.release();

    return true;
}

void JavaViewer::close() {
    delete o;
    o = nullptr;

    for(auto i: meshes)
        delete i.second;
}

void JavaViewer::press(const Math::Vector2<GLsizei>& position) {
    previousPosition = positionOnSphere(position);
}

void JavaViewer::drag(const Math::Vector2<GLsizei>& position) {
    if(!o) return;

    Vector3 currentPosition = positionOnSphere(position);

    Vector3 axis = Vector3::cross(previousPosition, currentPosition);

    if(previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

    GLfloat angle = acos(Vector3::dot(previousPosition, currentPosition));
    if(o) o->rotate(angle, axis);

    previousPosition = currentPosition;
}

void JavaViewer::release() {
    previousPosition = {};
}

void JavaViewer::zoom(int direction) {
    /* Distance between origin and near camera clipping plane */
    GLfloat distance = _camera.transformation()[3].z()-0-_camera.near();

    /* Move 15% of the distance back or forward */
    if(direction < 0)
        distance *= 1 - 1/0.85f;
    else
        distance *= 1 - 0.85f;
    _camera.translate(Vector3::zAxis(distance), Object::Transformation::Local);
}

Vector3 JavaViewer::positionOnSphere(const Math::Vector2<GLsizei>& position) const {
    Math::Vector2<GLsizei> viewport = _camera.viewport();
    Vector2 position_(position.x()*2.0f/viewport.x() - 1.0f,
                      position.y()*2.0f/viewport.y() - 1.0f);

    GLfloat length = position_.length();
    Vector3 result(length > 1.0f ? Vector3(position_, 0.0f) : Vector3(position_, 1.0f - length));
    result.setY(-result.y());
    return result.normalized();
}

void JavaViewer::addObject(AbstractImporter* colladaImporter, Object* parent, unordered_map<size_t, PhongMaterialData*>& materials, size_t objectId) {
    ObjectData* object = colladaImporter->object(objectId);

    /* Only meshes for now */
    if(object->instanceType() == ObjectData::InstanceType::Mesh) {

        /* Use already processed mesh, if exists */
        IndexedMesh* mesh;
        auto found = meshes.find(object->instanceId());
        if(found != meshes.end()) mesh = found->second;

        /* Or create a new one */
        else {
            mesh = new IndexedMesh;
            meshes.insert(make_pair(object->instanceId(), mesh));

            MeshData* data = colladaImporter->mesh(object->instanceId());
            if(!data || !data->indices() || !data->vertexArrayCount() || !data->normalArrayCount()) {
                Warning() << "Mesh data don't have expected features";
                return;
            }

            /* Interleave mesh data */
            Buffer* buffer = mesh->addBuffer(true);
            mesh->bindAttribute<PhongShader::Vertex>(buffer);
            mesh->bindAttribute<PhongShader::Normal>(buffer);
            MeshTools::interleave(mesh, buffer, Buffer::Usage::StaticDraw, *data->vertices(0), *data->normals(0));

            /* Compress indices */
            MeshTools::compressIndices(mesh, Buffer::Usage::StaticDraw, *data->indices());
        }

        /* Use already processed material, if exists */
        PhongMaterialData* material;
        auto materialFound = materials.find(static_cast<MeshObjectData*>(object)->material());
        if(materialFound != materials.end()) material = materialFound->second;

        /* Else get material or create default one */
        else {
            material = static_cast<PhongMaterialData*>(colladaImporter->material(static_cast<MeshObjectData*>(object)->material()));
            if(!material) material = new PhongMaterialData({0.0f, 0.0f, 0.0f}, {0.9f, 0.9f, 0.9f}, {1.0f, 1.0f, 1.0f}, 50.0f);
        }

        /* Add object */
        Object* o = new ViewedObject(mesh, material, &_shader, parent);
        o->setTransformation(object->transformation());
    }

    /* Recursively add children */
    for(size_t id: object->children())
        addObject(colladaImporter, o, materials, id);
}

}}
