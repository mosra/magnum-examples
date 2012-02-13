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
#include <chrono>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "Utility/Debug.h"
#include "Utility/Directory.h"
#include "PluginManager/PluginManager.h"

#include "Scene.h"
#include "Trade/AbstractImporter.h"
#include "MeshTools/Tipsify.h"

#include "configure.h"

using namespace std;
using namespace Corrade::PluginManager;
using namespace Corrade::Utility;
using namespace Magnum;

Scene* s;
Camera* camera;
shared_ptr<Object> o;
chrono::high_resolution_clock::time_point before;
bool wireframe, fps;
size_t frames;
double totalfps;
size_t totalmeasurecount;
Vector3 previousPosition;

/* Wrapper functions so GLUT can handle that */
void setViewport(int w, int h) {
    camera->setViewport(w, h);
}
void draw() {
    if(fps) {
        chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();
        double duration = chrono::duration<double>(now-before).count();
        if(duration > 3.5) {
            cout << frames << " frames in " << duration << " sec: "
                 << frames/duration << " FPS         \r";
            cout.flush();
            totalfps += frames/duration;
            before = now;
            frames = 0;
            ++totalmeasurecount;
        }
    }
    s->draw(camera);
    glutSwapBuffers();

    if(fps) {
        ++frames;
        glutPostRedisplay();
    }
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
            camera->translate(0, 0, -0.5);
            break;
        case GLUT_KEY_PAGE_DOWN:
            camera->translate(0, 0, 0.5);
            break;
        case GLUT_KEY_HOME:
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_FILL : GL_LINE);
            wireframe = !wireframe;
            break;
        case GLUT_KEY_END:
            if(fps) cout << "Average FPS on " << camera->viewport().x()
                << 'x' << camera->viewport().y() << " from "
                << totalmeasurecount << " measures: "
                << totalfps/totalmeasurecount << "          " << endl;
            else before = chrono::high_resolution_clock::now();

            fps = !fps;
            frames = totalmeasurecount = 0;
            totalfps = 0;
            break;
    }

    glutPostRedisplay();
}

Vector3 positionOnSphere(int x, int y) {
    Math::Vector2<unsigned int> viewport = camera->viewport();
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
            GLfloat distance = camera->transformation().at(3).z()-0-camera->near();

            /* Move 15% of the distance back or forward */
            if(button == 3)
                distance *= 1 - 1/0.85f;
            else
                distance *= 1 - 0.85f;
            camera->translate(0, 0, distance);

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
    PluginManager<Trade::AbstractImporter> manager(PLUGIN_IMPORTER_DIR);
    if(manager.load("ColladaImporter") != AbstractPluginManager::LoadOk) {
        Error() << "Could not load ColladaImporter plugin";
        return 2;
    }
    unique_ptr<Trade::AbstractImporter> colladaImporter(manager.instance("ColladaImporter"));
    if(!colladaImporter) {
        Error() << "Could not instance ColladaImporter plugin";
        return 3;
    }
    if(!(colladaImporter->features() & Trade::AbstractImporter::OpenFile)) {
        Error() << "ColladaImporter cannot open files";
        return 7;
    }

    /* Init GLUT */
    glutInit(&argc, argv);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
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
    fps = false;
    scene.setFeature(Scene::DepthTest, true);

    /* Every scene needs a camera */
    camera = new Camera(&scene);
    camera->setPerspective(35.0f*PI/180, 0.001f, 100);
    camera->translate(0, 0, 5);

    /* Load file */
    if(!colladaImporter->open(argv[1]))
        return 4;

    if(colladaImporter->objectCount() == 0)
        return 5;

    /* Optimize vertices */
    shared_ptr<Trade::AbstractImporter::MeshData> meshData = colladaImporter->meshData(0);
    if(meshData && meshData->indices() && meshData->vertices(0)) {
        Debug() << "Optimizing mesh vertices using Tipsify algorithm (cache size 24)...";
        MeshTools::tipsify(*meshData->indices(), meshData->vertices(0)->size(), 24);
    }

    o = colladaImporter->object(0);
    if(!o) return 6;
    o->setParent(&scene);

    colladaImporter->close();
    delete colladaImporter.release();

    /* Main loop calls draw() periodically and setViewport() on window size change */
    glutMainLoop();

    o.reset();

    return 0;
}
