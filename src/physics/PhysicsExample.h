#ifndef Magnum_Examples_PhysicsExample_h
#define Magnum_Examples_PhysicsExample_h
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

#include <SceneGraph/Scene.h>
#include <Physics/DebugDrawResourceManager.h>
#include <Physics/ShapedObjectGroup.h>

#ifndef MAGNUM_TARGET_GLES
#include <Contexts/GlutWindowContext.h>
#else
#include <Contexts/XEglWindowContext.h>
#endif

namespace Magnum { namespace Examples {

#ifndef MAGNUM_TARGET_GLES
typedef Contexts::GlutWindowContext WindowContext;
#else
typedef Contexts::XEglWindowContext WindowContext;
#endif

class PhysicsExample: public WindowContext {
    public:
        PhysicsExample(int& argc, char** argv);

    protected:
        void viewportEvent(const Math::Vector2<GLsizei>& size);
        void drawEvent();

    private:
        Physics::DebugDrawResourceManager resourceManager;
        Physics::ShapedObjectGroup2D group;
        SceneGraph::Scene2D scene;
        SceneGraph::Camera2D* camera;
};

}}

#endif
