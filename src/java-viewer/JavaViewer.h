#ifndef Magnum_Examples_JavaViewer_h
#define Magnum_Examples_JavaViewer_h
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

#include <unordered_map>
#include <PluginManager/PluginManager.h>
#include <Scene.h>
#include <Camera.h>
#include <Trade/AbstractImporter.h>
#include <Trade/PhongMaterialData.h>

#include "PhongShader.h"

namespace Magnum {

class IndexedMesh;

namespace Examples {

class JavaViewer {
    public:
        JavaViewer();

        ~JavaViewer();

        bool openCollada(const std::string& filename);
        void close();

        void press(const Math::Vector2<GLsizei>& position);
        void drag(const Math::Vector2<GLsizei>& position);
        void release();
        void zoom(int direction);

        inline PhongShader* shader() {
            return &_shader;
        }

        inline Camera* camera() {
            return &_camera;
        }

        inline void drawEvent() {
            _camera.draw();
        }

        inline void viewportEvent(const Math::Vector2<GLsizei>& size) {
            _camera.setViewport(size);
        }

    private:
        Scene scene;
        Camera _camera;
        Corrade::PluginManager::PluginManager<Trade::AbstractImporter> manager;
        PhongShader _shader;
        Object* o;
        std::unordered_map<size_t, IndexedMesh*> meshes;
        Vector3 previousPosition;

        Vector3 positionOnSphere(const Math::Vector2<GLsizei>& position) const;

        void addObject(Trade::AbstractImporter* colladaImporter, Object* parent, std::unordered_map<size_t, Trade::PhongMaterialData*>& materials, size_t objectId);
};

}}

#endif
