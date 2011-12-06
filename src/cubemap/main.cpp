/*
    Copyright © 2010, 2011 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "PluginManager/PluginManager.h"

#include "Scene.h"
#include "AbstractImporter.h"

#include "CubeMap.h"
#include "configure.h"

using namespace std;
using namespace Corrade::PluginManager;
using namespace Corrade::Utility;
using namespace Magnum;
using namespace Magnum::Examples;

Scene* s;
Camera* camera;

/* Wrapper functions so GLUT can handle that */
void setViewport(int w, int h) {
    s->setViewport(w, h);
}
void draw() {
    s->draw();
    glutSwapBuffers();
}

void event(int key, int x, int y) {
    if(key == GLUT_KEY_UP)
        camera->rotate(deg(-10.0f), camera->transformation().at(0).xyz(), true);

    if(key == GLUT_KEY_DOWN)
        camera->rotate(deg(10.0f), camera->transformation().at(0).xyz(), true);

    if(key == GLUT_KEY_LEFT || key == GLUT_KEY_RIGHT) {
        GLfloat yTransform = camera->transformation().at(2, 3);
        camera->translate(0, -yTransform, 0);
        if(key == GLUT_KEY_LEFT)
            camera->rotate(deg(10.0f), Vector3::yAxis(), true);
        else
            camera->rotate(deg(-10.0f), Vector3::yAxis(), true);
        camera->translate(0, yTransform, 0);
    }

    glutPostRedisplay();
}

int main(int argc, char** argv) {
    /* Init GLUT */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Cube Map example");
    glutReshapeFunc(setViewport);
    glutDisplayFunc(draw);
    glutSpecialFunc(event);

    /* Init GLEW */
    GLenum err = glewInit();
    if(err != GLEW_OK) {
        Corrade::Utility::Error() << "GLEW error:" << glewGetErrorString(err);
        return 1;
    }

    /* Initialize scene */
    Scene scene;
    s = &scene;
    scene.setFeature(Magnum::Scene::DepthTest, true);

    /* Every scene needs a camera */
    camera = new Camera(&scene);
    scene.setCamera(camera);
    camera->setPerspective(deg(75), 0.001f, 100);
    camera->translate(0, 0, 3);

    /* Load TGA importer plugin */
    PluginManager<AbstractImporter> manager(PLUGIN_IMPORTER_DIR);
    AbstractImporter* importer;
    if(manager.load("TGAImporter") != AbstractPluginManager::LoadOk || !(importer = manager.instance("TGAImporter"))) {
        Error() << "Cannot load TGAImporter plugin from" << PLUGIN_IMPORTER_DIR;
        return 2;
    }

    string prefix;
    if(argc == 2)
        prefix = argv[1];

    /* Add cube map to the scene */
    new CubeMap(importer, prefix, &scene);

    delete importer;

    glutMainLoop();
    return 0;
}
