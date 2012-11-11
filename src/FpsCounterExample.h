#ifndef MAGNUM_EXAMPLES_FpsCounterExample_h
#define MAGNUM_EXAMPLES_FpsCounterExample_h
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

#include <chrono>
#include <Query.h>

#ifndef MAGNUM_TARGET_GLES
#include <Platform/GlutApplication.h>
#else
#include <Platform/XEglApplication.h>
#endif

namespace Magnum { namespace Examples {

#ifndef MAGNUM_TARGET_GLES
typedef Platform::GlutApplication Application;
#else
typedef Platform::XEglApplication Application;
#endif

/** @brief Base class for examples with FPS counter */
class FpsCounterExample: public Application {
    public:
        FpsCounterExample(int& argc, char** argv, const std::string& name = "Magnum Example", const Math::Vector2<GLsizei>& size = Math::Vector2<GLsizei>(800, 600));

        /**
         * @brief Minimal duration between printing FPS to console
         *
         * Default value is 3.5 seconds.
         */
        inline double minimalCounterDuration() const { return minimalDuration; }

        /**
         * @brief Set minimal duration between printing FPS to console output
         *
         * The larger duration, the more precise and steady FPS information
         * will be.
         */
        inline void setMinimalCounterDuration(double value) {
            minimalDuration = value;
        }

        /** @brief Whether FPS counter is enabled */
        inline bool fpsCounterEnabled() const { return fpsEnabled; }

        /**
         * @brief Enable or disable FPS counter
         *
         * The counter is off by default. If enabled, calls reset().
         */
        void setFpsCounterEnabled(bool enabled);

        #ifndef MAGNUM_TARGET_GLES
        /** @brief Whether primitive counter is enabled */
        inline bool primitiveCounterEnabled() const { return primitiveEnabled; }

        /**
         * @brief Enable or disable primitive counter
         *
         * The counter is off by default. If enabled, calls resetCounter().
         * If disabled, disables also sample counter.
         */
        void setPrimitiveCounterEnabled(bool enabled);

        /** @brief Whether sample counter is enabled */
        inline bool sampleCounterEnabled() const { return sampleEnabled; }

        /**
         * @brief Enable or disable sample counter
         *
         * Counts number of samples per primitive. The counter is off by
         * default. If enabled, calls resetCounter() and enables also
         * primitive counter.
         */
        void setSampleCounterEnabled(bool enabled);
        #endif

        /**
         * @brief Reset counter
         *
         * Call after operations which would spoil FPS information, such as
         * very long frames, scene changes etc. This function is called
         * automatically in viewportEvent().
         */
        void resetCounter();

        /**
         * @brief Print FPS statistics for longer duration
         *
         * This function prints FPS statistics counted from the beginning or
         * previous time statistics() was called and then calls reset().
         */
        void printCounterStatistics();

    protected:
        /**
         * @copydoc GlutContext::viewportEvent()
         *
         * Calls reset() and saves viewport size.
         * @attention You have to call this function from your viewportEvent()
         * reimplementation!
         */
        void viewportEvent(const Math::Vector2<GLsizei>& size) {
            resetCounter();
            viewport = size;
        }

        /**
         * @copydoc GlutContext::redraw()
         *
         * Measures FPS in given duration and prints statistics to
         * console.
         */
        void redraw();

    private:
        size_t frames, totalFrames;
        double minimalDuration, totalDuration;
        bool fpsEnabled;
        #ifndef MAGNUM_TARGET_GLES
        GLuint primitives, totalPrimitives, samples, totalSamples;
        bool primitiveEnabled, sampleEnabled;
        Query primitiveQuery;
        SampleQuery sampleQuery;
        #endif
        std::chrono::high_resolution_clock::time_point before;
        Math::Vector2<GLsizei> viewport;
};

}}

#endif
