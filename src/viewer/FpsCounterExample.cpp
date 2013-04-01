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

#include "FpsCounterExample.h"

#include <iostream>
#include <iomanip>

using namespace Magnum;

namespace Magnum { namespace Examples {

FpsCounterExample::FpsCounterExample(const Arguments& arguments, Configuration* configuration): Application(arguments, configuration), frames(0), totalFrames(0), minimalDuration(3.5), totalDuration(0.0), fpsEnabled(false)
    #ifndef MAGNUM_TARGET_GLES
    , primitives(0), totalPrimitives(0), samples(0), totalSamples(0), primitiveEnabled(false), sampleEnabled(false)
    #endif
{}

void FpsCounterExample::redraw() {
    #ifndef MAGNUM_TARGET_GLES
    if(fpsEnabled || primitiveEnabled || sampleEnabled) {
    #else
    if(fpsEnabled) {
    #endif
        if(fpsEnabled)
            ++frames;
        #ifndef MAGNUM_TARGET_GLES
        if(primitiveEnabled) {
            primitiveQuery.end();
            primitives += primitiveQuery.result<UnsignedInt>();
            primitiveQuery.begin(Query::Target::PrimitivesGenerated);
        }
        if(sampleEnabled) {
            sampleQuery.end();
            samples += sampleQuery.result<UnsignedInt>();
            sampleQuery.begin(SampleQuery::Target::SamplesPassed);
        }
        #endif

        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double>(now-before).count();
        if(duration > minimalDuration) {
            std::streamsize precision = std::cout.precision();
            std::ostream::fmtflags flags = std::cout.flags();
            std::cout.precision(1);
            std::cout.setf(std::ostream::floatfield, std::ostream::fixed);
            if(fpsEnabled) {
                std::cout << std::setw(10) << frames/duration << " FPS ";
                totalFrames += frames;
            }
            #ifndef MAGNUM_TARGET_GLES
            if(primitiveEnabled) {
                std::cout << std::setw(10) << double(primitives)/frames << " tris/frame ";
                std::cout << std::setw(10) << primitives/(duration*1000) << "k tris/s ";
                totalPrimitives += primitives;
            }
            if(sampleEnabled) {
                std::cout << std::setw(10) << double(samples)/primitives << " samples/tri";
                totalSamples += samples;
            }
            #endif

            std::cout << '\r';
            std::cout.precision(precision);
            std::cout.flags(flags);
            std::cout.flush();

            frames = 0;
            #ifndef MAGNUM_TARGET_GLES
            primitives = 0;
            samples = 0;
            #endif
            before = now;
            totalDuration += duration;
        }
    }

    Application::redraw();
}

void FpsCounterExample::setFpsCounterEnabled(bool enabled) {
    if(fpsEnabled == enabled) return;

    resetCounter();
    fpsEnabled = enabled;
}

#ifndef MAGNUM_TARGET_GLES
void FpsCounterExample::setPrimitiveCounterEnabled(bool enabled) {
    if(primitiveEnabled == enabled) return;

    resetCounter();

    if(enabled)
        primitiveQuery.begin(Query::Target::PrimitivesGenerated);
    else {
        setSampleCounterEnabled(false);
        primitiveQuery.end();
    }

    primitiveEnabled = enabled;
}

void FpsCounterExample::setSampleCounterEnabled(bool enabled) {
    if(sampleEnabled == enabled) return;

    resetCounter();

    if(enabled) {
        setPrimitiveCounterEnabled(true);
        sampleQuery.begin(SampleQuery::Target::SamplesPassed);
    } else sampleQuery.end();

    sampleEnabled = enabled;
}
#endif

void FpsCounterExample::resetCounter() {
    before = std::chrono::high_resolution_clock::now();
    frames = totalFrames = 0;
    #ifndef MAGNUM_TARGET_GLES
    primitives = totalPrimitives = samples = totalSamples = 0;
    #endif
    totalDuration = 0.0;
}

void FpsCounterExample::printCounterStatistics() {
    if(totalDuration != 0) {
        std::streamsize precision = std::cout.precision();
        std::ostream::fmtflags flags = std::cout.flags();
        std::cout.precision(1);
        std::cout.setf(std::ostream::floatfield, std::ostream::fixed);
        std::cout << "Average values on " << viewport.x()
             << "x" << viewport.y() << " during " << totalDuration
             << " seconds:                                 \n";
        if(fpsEnabled)
            std::cout << std::setw(10) << totalFrames/totalDuration << " FPS ";

        #ifndef MAGNUM_TARGET_GLES
        if(primitiveEnabled) {
            std::cout << std::setw(10) << double(totalPrimitives)/totalFrames << " tris/frame ";
            std::cout << std::setw(10) << totalPrimitives/(totalDuration*1000) << "k tris/s ";
        }
        if(sampleEnabled)
            std::cout << std::setw(10) << double(totalSamples)/totalPrimitives << " samples/tri";
        #endif

        std::cout.precision(precision);
        std::cout.flags(flags);
        std::cout << std::endl;
    }

    resetCounter();
}

}}
