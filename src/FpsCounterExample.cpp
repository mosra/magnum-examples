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

#include "FpsCounterExample.h"

#include <iostream>
#include <iomanip>

using namespace std;
using namespace Magnum;

namespace Magnum { namespace Examples {

FpsCounterExample::FpsCounterExample(int& argc, char** argv, const string& name, const Math::Vector2<GLsizei>& size): Application(argc, argv, name, size), frames(0), totalFrames(0), minimalDuration(3.5), totalDuration(0.0), fpsEnabled(false)
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
            primitives += primitiveQuery.result<GLuint>();
            primitiveQuery.begin(Query::Target::PrimitivesGenerated);
        }
        if(sampleEnabled) {
            sampleQuery.end();
            samples += sampleQuery.result<GLuint>();
            sampleQuery.begin(SampleQuery::Target::SamplesPassed);
        }
        #endif

        chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();
        double duration = chrono::duration<double>(now-before).count();
        if(duration > minimalDuration) {
            streamsize precision = cout.precision();
            ostream::fmtflags flags = cout.flags();
            cout.precision(1);
            cout.setf(ostream::floatfield, ostream::fixed);
            if(fpsEnabled) {
                cout << setw(10) << frames/duration << " FPS ";
                totalFrames += frames;
            }
            #ifndef MAGNUM_TARGET_GLES
            if(primitiveEnabled) {
                cout << setw(10) << double(primitives)/frames << " tris/frame ";
                cout << setw(10) << primitives/(duration*1000) << "k tris/s ";
                totalPrimitives += primitives;
            }
            if(sampleEnabled) {
                cout << setw(10) << double(samples)/primitives << " samples/tri";
                totalSamples += samples;
            }
            #endif

            cout << '\r';
            cout.precision(precision);
            cout.flags(flags);
            cout.flush();

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
    before = chrono::high_resolution_clock::now();
    frames = totalFrames = 0;
    #ifndef MAGNUM_TARGET_GLES
    primitives = totalPrimitives = samples = totalSamples = 0;
    #endif
    totalDuration = 0.0;
}

void FpsCounterExample::printCounterStatistics() {
    if(totalDuration != 0) {
        streamsize precision = cout.precision();
        ostream::fmtflags flags = cout.flags();
        cout.precision(1);
        cout.setf(ostream::floatfield, ostream::fixed);
        cout << "Average values on " << viewport.x()
             << 'x' << viewport.y() << " during " << totalDuration
             << " seconds:                                 \n";
        if(fpsEnabled)
            cout << setw(10) << totalFrames/totalDuration << " FPS ";

        #ifndef MAGNUM_TARGET_GLES
        if(primitiveEnabled) {
            cout << setw(10) << double(totalPrimitives)/totalFrames << " tris/frame ";
            cout << setw(10) << totalPrimitives/(totalDuration*1000) << "k tris/s ";
        }
        if(sampleEnabled)
            cout << setw(10) << double(totalSamples)/totalPrimitives << " samples/tri";
        #endif

        cout.precision(precision);
        cout.flags(flags);
        cout << endl;
    }

    resetCounter();
}

}}
