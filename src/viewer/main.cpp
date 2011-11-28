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
#include <fstream>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "Utility/Debug.h"
#include "Utility/Directory.h"
#include "PluginManager/PluginManager.h"

#include "Scene.h"
#include "AbstractImporter.h"

#include "configure.h"

using namespace std;
using namespace Corrade::PluginManager;
using namespace Corrade::Utility;
using namespace Magnum;

Scene* s;
shared_ptr<Object> o;

/* Wrapper functions so GLUT can handle that */
void setViewport(int w, int h) {
    s->setViewport(w, h);
}
void draw() {
    s->draw();
    glutSwapBuffers();
}

void events(int key, int x, int y) {
    switch(key) {
        case GLUT_KEY_UP:
            o->rotate(PI/18, -1, 0, 0);
            break;
        case GLUT_KEY_DOWN:
            o->rotate(PI/18, 1, 0, 0);
            break;
        case GLUT_KEY_LEFT:
            o->rotate(PI/18, 0, -1, 0, false);
            break;
        case GLUT_KEY_RIGHT:
            o->rotate(PI/18, 0, 1, 0, false);
            break;
        case GLUT_KEY_PAGE_UP:
            s->camera()->translate(0, 0, -0.5);
            break;
        case GLUT_KEY_PAGE_DOWN:
            s->camera()->translate(0, 0, 0.5);
            break;
    }

    glutPostRedisplay();
}

int main(int argc, char** argv) {
    if(argc != 2) {
        cout << "Usage: " << argv[0] << " file.dae" << endl;
        return 0;
    }

    /* Instance ColladaImporter plugin */
    PluginManager<AbstractImporter> manager(PLUGIN_IMPORTER_DIR);
    if(manager.load("ColladaImporter") != AbstractPluginManager::LoadOk) {
        Error() << "Could not load ColladaImporter plugin";
        return 2;
    }
    unique_ptr<AbstractImporter> colladaImporter(manager.instance("ColladaImporter"));
    if(!colladaImporter) {
        Error() << "Could not instance ColladaImporter plugin";
        return 3;
    }

    /* Init GLUT */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Magnum viewer");
    glutReshapeFunc(setViewport);
    glutSpecialFunc(events);
    glutDisplayFunc(draw);

    /* Init GLEW */
    GLenum err = glewInit();
    if(err != GLEW_OK) {
        Error() << "GLEW error:" << glewGetErrorString(err);
        return 1;
    }

    /* Initialize scene */
    Scene scene;
    s = &scene;
    scene.setFeature(Scene::DepthTest, true);

    /* Every scene needs a camera */
    Camera* camera = new Camera(&scene);
    camera->setPerspective(35.0f*PI/180, 0.001f, 100);
    camera->translate(0, 0, 5);
    scene.setCamera(camera);

    /* Load file */
    ifstream in(argv[1]);
    if(!colladaImporter->open(in))
        return 4;

    if(colladaImporter->objectCount() == 0)
        return 5;

    o = colladaImporter->object(0);
    if(!o) return 6;
    o->setParent(&scene);

    colladaImporter->close();
    delete colladaImporter.release();
    in.close();

    /* Main loop calls draw() periodically and setViewport() on window size change */
    glutMainLoop();
    return 0;
}
