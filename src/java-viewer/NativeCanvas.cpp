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

#include <Magnum.h>
#include <GL/glx.h>
#include <jawt_md.h>

#include "NativeCanvas.h"
#include "JavaViewer.h"

using namespace std;
using namespace Magnum;
using namespace Magnum::Examples;
using namespace Corrade::Utility;

JAWT awt;
GLXContext context;
Display *display;
GLXDrawable window;

JavaViewer* viewer = nullptr;

JNIEXPORT void JNICALL Java_cz_mosra_magnum_JavaViewer_NativeCanvas_construct(JNIEnv *env, jobject panel) {
    XInitThreads();

    /* Get display and window from X11 drawing surface infor provided by AWT */
    awt.version = JAWT_VERSION_1_3;
    if(JAWT_GetAWT(env, &awt) == JNI_FALSE) {
        Error() << "Cannot get AWT";
        exit(1);
    }
    JAWT_DrawingSurface* drawingSurface = awt.GetDrawingSurface(env, panel);
    drawingSurface->Lock(drawingSurface);
    JAWT_X11DrawingSurfaceInfo *xDrawingSurfaceInfo;
    xDrawingSurfaceInfo = reinterpret_cast<JAWT_X11DrawingSurfaceInfo*>(drawingSurface->GetDrawingSurfaceInfo(drawingSurface)->platformInfo);
    if(!(display = xDrawingSurfaceInfo->display)) {
        Error() << "Cannot get display";
        exit(1);
    }
    if(!(window = xDrawingSurfaceInfo->drawable)) {
        Error() << "Cannot get window";
        exit(1);
    }

    /* Get visual from framebuffer configuration */
    int fbCount;
    static const int fbAttribs[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, true,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        0
    };
    GLXFBConfig *fbConfig = glXChooseFBConfig(display, DefaultScreen(display), fbAttribs, &fbCount);
    if(!fbConfig) {
        Error() << "Cannot choose framebuffer configuration";
        exit(1);
    }
    XVisualInfo *visualInfo = glXGetVisualFromFBConfig(display, fbConfig[0]);

    /* Create OpenGL context */
    if(!(context = glXCreateContext(display, visualInfo, 0, true))) {
        Error() << "Cannot create context";
        exit(1);
    }

    /* Initialize viewer */
    glXMakeCurrent(display, window, context);

    /* Init GLEW */
    GLenum err = glewInit();
    if(err != GLEW_OK) {
        Error() << "Cannot initialize GLEW:" << glewGetErrorString(err);
        exit(1);
    }

    viewer = new JavaViewer;

    drawingSurface->Unlock(drawingSurface);
}

JNIEXPORT void JNICALL Java_cz_mosra_magnum_JavaViewer_NativeCanvas_destruct(JNIEnv* env, jobject) {
    delete viewer;
    viewer = nullptr;

    glXDestroyContext(display, context);
}

JNIEXPORT jboolean JNICALL Java_cz_mosra_magnum_JavaViewer_JavaViewer_openCollada(JNIEnv* env, jobject, jstring filename) {
    CORRADE_ASSERT(viewer, "Unable to access native implementation instance", false)

    const char* filename_ = env->GetStringUTFChars(filename, nullptr);
    std::string filename__(filename_);
    env->ReleaseStringUTFChars(filename, filename_);

    return viewer->openCollada(filename__);
}

JNIEXPORT void JNICALL Java_cz_mosra_magnum_JavaViewer_JavaViewer_close(JNIEnv*, jobject) {
    CORRADE_ASSERT(viewer, "Unable to access native implementation instance", )

    viewer->close();
}

JNIEXPORT void JNICALL Java_cz_mosra_magnum_JavaViewer_JavaViewer_setLightColor(JNIEnv*, jobject, jint id, jfloat r, jfloat g, jfloat b) {
    CORRADE_ASSERT(viewer, "Unable to access native implementation instance", )

    if(id >= 0 && id < 3)
        viewer->shader()->setLightColorUniform(id, {r, g, b});
    else if(id == 3)
        viewer->camera()->setClearColor({r, g, b});
}

JNIEXPORT void JNICALL Java_cz_mosra_magnum_JavaViewer_NativeCanvas_setViewport(JNIEnv* env, jobject canvas, jint width, jint height) {
    CORRADE_ASSERT(viewer, "Unable to access native implementation instance" << width << height, )

    JAWT_DrawingSurface* drawingSurface = awt.GetDrawingSurface(env, canvas);
    drawingSurface->Lock(drawingSurface);

    glXMakeCurrent(display, window, context);
    viewer->viewportEvent({width, height});

    drawingSurface->Unlock(drawingSurface);
}

JNIEXPORT void JNICALL Java_cz_mosra_magnum_JavaViewer_NativeCanvas_press(JNIEnv*, jobject, jint x, jint y) {
    CORRADE_ASSERT(viewer, "Unable to access native implementation instance", )

    viewer->press({x, y});
}

JNIEXPORT void JNICALL Java_cz_mosra_magnum_JavaViewer_NativeCanvas_drag(JNIEnv*, jobject, jint x, jint y) {
    CORRADE_ASSERT(viewer, "Unable to access native implementation instance", )

    viewer->drag({x, y});
}

JNIEXPORT void JNICALL Java_cz_mosra_magnum_JavaViewer_NativeCanvas_release(JNIEnv*, jobject) {
    CORRADE_ASSERT(viewer, "Unable to access native implementation instance", )

    viewer->release();
}

JNIEXPORT void JNICALL Java_cz_mosra_magnum_JavaViewer_NativeCanvas_zoom(JNIEnv*, jobject, jint direction) {
    CORRADE_ASSERT(viewer, "Unable to access native implementation instance", )

    viewer->zoom(direction);
}

JNIEXPORT void JNICALL Java_cz_mosra_magnum_JavaViewer_NativeCanvas_draw(JNIEnv* env, jobject canvas) {
    CORRADE_ASSERT(viewer, "Unable to access native implementation instance", )

    JAWT_DrawingSurface* drawingSurface = awt.GetDrawingSurface(env, canvas);
    drawingSurface->Lock(drawingSurface);

    glXMakeCurrent(display, window, context);
    viewer->drawEvent();

    glXSwapBuffers(display, window);

    drawingSurface->Unlock(drawingSurface);
}
