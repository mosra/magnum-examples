#ifndef MAGNUM_EXAMPLES_FpsCounterExample_h
#define MAGNUM_EXAMPLES_FpsCounterExample_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
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
        explicit FpsCounterExample(const Arguments& arguments, const Configuration& configuration);

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
        void viewportEvent(const Vector2i& size) override {
            resetCounter();
            viewport = size;
        }

        /**
         * @copydoc GlutContext::redraw()
         *
         * Measures FPS in given duration and prints statistics to
         * console.
         */
        void redraw() override;

    private:
        std::size_t frames, totalFrames;
        double minimalDuration, totalDuration;
        bool fpsEnabled;
        #ifndef MAGNUM_TARGET_GLES
        UnsignedInt primitives, totalPrimitives, samples, totalSamples;
        bool primitiveEnabled, sampleEnabled;
        PrimitiveQuery primitiveQuery;
        SampleQuery sampleQuery;
        #endif
        std::chrono::high_resolution_clock::time_point before;
        Vector2i viewport;
};

}}

#endif
