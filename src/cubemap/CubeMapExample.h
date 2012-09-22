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

#include <Contexts/GlutWindowContext.h>
#include <SceneGraph/Scene.h>

#include "CubeMapResourceManager.h"

namespace Magnum { namespace Examples {

class CubeMapExample: public Contexts::GlutWindowContext {
    public:
        CubeMapExample(int& argc, char** argv);

    protected:
        void viewportEvent(const Math::Vector2<GLsizei>& size);
        void drawEvent();

        void keyPressEvent(Key key, const Math::Vector2<int>& position);

    private:
        CubeMapResourceManager resourceManager;
        SceneGraph::Scene3D scene;
        SceneGraph::Camera3D* camera;
};

}}
