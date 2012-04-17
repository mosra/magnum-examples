#ifndef Magnum_Examples_AbstractExample_h
#define Magnum_Examples_AbstractExample_h
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

#include "Magnum.h"

#include <GL/freeglut.h>

namespace Magnum { namespace Examples {

/**
@brief Base class for Magnum examples

You need to reimplement at least drawEvent() and viewportEvent() to be able
to draw on the screen. See also MAGNUM_EXAMPLE_MAIN() macro, which creates
simple <tt>main()</tt> function for your example class.
*/
class AbstractExample {
    public:
        /** @brief Key */
        enum class Key: int {
            Up = GLUT_KEY_UP,               /**< Up arrow */
            Down = GLUT_KEY_DOWN,           /**< Down arrow */
            Left = GLUT_KEY_LEFT,           /**< Left arrow */
            Right = GLUT_KEY_RIGHT,         /**< Right arrow */
            F1 = GLUT_KEY_F1,               /**< F1 */
            F2 = GLUT_KEY_F2,               /**< F2 */
            F3 = GLUT_KEY_F3,               /**< F3 */
            F4 = GLUT_KEY_F4,               /**< F4 */
            F5 = GLUT_KEY_F5,               /**< F5 */
            F6 = GLUT_KEY_F6,               /**< F6 */
            F7 = GLUT_KEY_F7,               /**< F7 */
            F8 = GLUT_KEY_F8,               /**< F8 */
            F9 = GLUT_KEY_F9,               /**< F9 */
            F10 = GLUT_KEY_F10,             /**< F10 */
            F11 = GLUT_KEY_F11,             /**< F11 */
            F12 = GLUT_KEY_F12,             /**< F12 */
            Home = GLUT_KEY_HOME,           /**< Home */
            End = GLUT_KEY_END,             /**< End */
            PageUp = GLUT_KEY_PAGE_UP,      /**< Page up */
            PageDown = GLUT_KEY_PAGE_DOWN   /**< Page down */
        };

        /** @brief Mouse button */
        enum class MouseButton: int {
            Left = GLUT_LEFT_BUTTON,        /**< Left button */
            Middle = GLUT_MIDDLE_BUTTON,    /**< Middle button */
            Right = GLUT_RIGHT_BUTTON,      /**< Right button */
            WheelUp = 3, /* glut wtf. */    /**< Wheel up */
            WheelDown = 4 /* glut wtf. */   /**< Wheel down */
        };

        /** @brief Mouse state */
        enum class MouseState: int {
            Up = GLUT_UP,                   /**< No button pressed */
            Down = GLUT_DOWN                /**< Button pressed */
        };

        /** @brief Mouse cursor */
        enum class MouseCursor: int {
            Default = GLUT_CURSOR_INHERIT,  /**< Default cursor provided by parent window */
            None = GLUT_CURSOR_NONE         /**< No cursor */
        };

        /**
         * @brief Constructor
         * @param argc      Count of arguments of <tt>main()</tt> function
         * @param argv      Arguments of <tt>main()</tt> function
         * @param name      Window title
         * @param size      Window size
         */
        AbstractExample(int& argc, char** argv, const std::string& name = "Magnum Example", const Math::Vector2<GLsizei>& size = Math::Vector2<GLsizei>(800, 600));

        /** @brief Destructor */
        virtual inline ~AbstractExample() {}

        /**
         * @brief Enable or disable mouse tracking
         *
         * When mouse tracking is enabled, mouseMoveEvent() is called even
         * when no button is pressed. Mouse tracking is disabled by default.
         */
        inline void setMouseTracking(bool enabled) {
            glutPassiveMotionFunc(enabled ? staticMouseMoveEvent : nullptr);
        }

        /** @brief Set mouse cursor */
        inline void setMouseCursor(MouseCursor cursor) {
            glutSetCursor(static_cast<int>(cursor));
        }

        /** @brief Warp mouse cursor to given coordinates */
        inline void warpMouseCursor(const Math::Vector2<GLsizei>& position) {
            glutWarpPointer(position.x(), position.y());
        }

        /** @brief Execute the main loop */
        inline int exec() {
            glutMainLoop();
            return 0;
        }

    protected:
        /**
         * @brief Viewport event
         *
         * Called when viewport size changes. You should pass the new size to
         * Camera::viewport() function of your camera.
         */
        virtual void viewportEvent(const Math::Vector2<GLsizei>& size) = 0;

        /**
         * @brief Key event
         *
         * Called when an key is pressed. Default implementation does nothing.
         */
        virtual inline void keyEvent(Key key, const Math::Vector2<int>& position) {}

        /**
         * @brief Mouse event
         *
         * Called when mouse button is pressed or released. Default
         * implementation does nothing.
         */
        virtual inline void mouseEvent(MouseButton button, MouseState state, const Math::Vector2<int>& position) {}

        /**
         * @brief Mouse move event
         *
         * Called when any mouse button is pressed and mouse is moved. Default
         * implementation does nothing.
         *
         * @see setMouseTracking()
         */
        virtual inline void mouseMoveEvent(const Math::Vector2<int>& position) {}

        /**
         * @brief Draw event
         *
         * Implement your drawing functions here. After drawing is finished,
         * call swapBuffers() or redraw(), if you want to draw immediately
         * again.
         */
        virtual void drawEvent() = 0;

        /**
         * @brief Swap buffers
         *
         * Paints currently rendered framebuffer on screen.
         */
        inline void swapBuffers() {
            glutSwapBuffers();
        }

        /**
         * @brief Redraw immediately
         *
         * Marks the window for redrawing, resulting in call of drawEvent()
         * in the next iteration.
         */
        virtual inline void redraw() {
            glutPostRedisplay();
        }

    private:
        inline static void staticViewportEvent(int x, int y) {
            instance->viewportEvent({x, y});
        }

        inline static void staticKeyEvent(int key, int x, int y) {
            instance->keyEvent(static_cast<Key>(key), {x, y});
        }

        inline static void staticMouseEvent(int button, int state, int x, int y) {
            instance->mouseEvent(static_cast<MouseButton>(button), static_cast<MouseState>(state), {x, y});
        }

        inline static void staticMouseMoveEvent(int x, int y) {
            instance->mouseMoveEvent({x, y});
        }

        inline static void staticDrawEvent() {
            instance->drawEvent();
        }

        static AbstractExample* instance;

        int& argc;
        char** argv;
};

/**
@brief Main function for given example class

Instantiates the class and calls AbstractExample::exec(). Note that your
subclass constructor must have exactly two required parameters, the same
as in AbstractExample::AbstractExample().
*/
#define MAGNUM_EXAMPLE_MAIN(class)                                          \
    int main(int argc, char** argv) {                                       \
        class e(argc, argv);                                                \
        return e.exec();                                                    \
    }

}}

#endif
