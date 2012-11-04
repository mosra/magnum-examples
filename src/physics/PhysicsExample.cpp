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

#include "PhysicsExample.h"

#include <Math/Constants.h>
#include <Framebuffer.h>
#include <Physics/Box.h>
#include <Physics/ShapedObject.h>
#include <Physics/ShapeGroup.h>
#include <SceneGraph/Camera.h>

namespace Magnum { namespace Examples {

PhysicsExample::PhysicsExample(int& argc, char** argv): WindowContext(argc, argv, "Physics example") {
    Framebuffer::setClearColor(Color3<GLfloat>::fromHSV(50.0f, 0.1f, 1.0f));

    Physics::DebugDrawResourceManager::Options* opt = new Physics::DebugDrawResourceManager::Options{Color3<GLfloat>::fromHSV(190.0f, 0.5f, 1.0f)};
    Physics::DebugDrawResourceManager::instance()->set<Physics::DebugDrawResourceManager::Options>("default", opt, ResourceDataState::Final, ResourcePolicy::Resident);

    Physics::ShapedObject2D* o = new Physics::ShapedObject2D(&group, &scene);
    o->setShape(new Physics::ShapeGroup2D(
        Physics::Box2D(Matrix3::scaling(Vector2(0.3f))) ||
        Physics::Box2D(Matrix3::translation(Vector2::xAxis(0.5f))*
                       Matrix3::rotation(deg(15.0f))*
                       Matrix3::scaling(Vector2(0.1f)))))
        ->rotate(deg(35.0f));
    Physics::DebugDrawResourceManager::createDebugRenderer(o->shape(), "default")
        ->setParent(o);

    camera = new SceneGraph::Camera2D(&scene);
    camera->setAspectRatioPolicy(SceneGraph::Camera2D::AspectRatioPolicy::Extend);
}

void PhysicsExample::viewportEvent(const Math::Vector2< GLsizei >& size) {
    Framebuffer::setViewport({}, size);

    camera->setViewport(size);
}

void PhysicsExample::drawEvent() {
    Framebuffer::clear(Framebuffer::Clear::Color);

    group.setClean();
    camera->draw();

    swapBuffers();
}

}}

int main(int argc, char** argv) {
    Magnum::Examples::PhysicsExample e(argc, argv);
    return e.exec();
}
