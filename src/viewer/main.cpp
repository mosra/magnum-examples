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
bool wireframe;
Vector3 previousPosition;

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
        case GLUT_KEY_HOME:
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_FILL : GL_LINE);
            wireframe = !wireframe;
            break;
    }

    glutPostRedisplay();
}

Vector3 positionOnSphere(int x, int y) {
    Math::Vector2<unsigned int> viewport = s->camera()->viewport();
    Vector2 position(x*2.0f/viewport.x() - 1.0f,
                     y*2.0f/viewport.y() - 1.0f);

    GLfloat length = position.length();
    Vector3 result(length > 1.0f ? position : Vector3(position, 1.0f - length));
    result.setY(-result.y());
    return result.normalized();
}

void mouseEvents(int button, int state, int x, int y) {
    switch(button) {
        case GLUT_LEFT_BUTTON:
            if(state == GLUT_DOWN) previousPosition = positionOnSphere(x, y);
            else previousPosition = Vector3();
            break;
        case 3:
        case 4:
            if(state == GLUT_UP) return;

            /* Distance between origin and near camera clipping plane */
            GLfloat distance = s->camera()->transformation().at(3).z()-0-s->camera()->near();

            /* Move 15% of the distance back or forward */
            if(button == 3)
                distance *= 1 - 1/0.85f;
            else
                distance *= 1 - 0.85f;
            s->camera()->translate(0, 0, distance);

            glutPostRedisplay();
            break;
    }
}

void dragEvents(int x, int y) {
    Vector3 currentPosition = positionOnSphere(x, y);

    Vector3 axis = Vector3::cross(previousPosition, currentPosition);

    if(previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

    GLfloat angle = acos(previousPosition*currentPosition);
    o->rotate(angle, axis);

    previousPosition = currentPosition;

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
    glutMouseFunc(mouseEvents);
    glutMotionFunc(dragEvents);
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
    wireframe = false;
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
