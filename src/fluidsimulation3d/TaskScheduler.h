#ifndef Magnum_Examples_FluidSimulation3D_TaskScheduler_h
#define Magnum_Examples_FluidSimulation3D_TaskScheduler_h
/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
        2019 — Nghia Truong <nghiatruong.vn@gmail.com>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "configure.h"

#ifdef MAGNUM_FLUIDSIMULATION3D_EXAMPLE_USE_MULTITHREADING
    #ifdef MAGNUM_FLUIDSIMULATION3D_EXAMPLE_USE_TBB
    #include <tbb/parallel_for.h>
    #else
    #include "ThreadPool.h"
    #endif
#endif

namespace Magnum { namespace Examples { namespace TaskScheduler {

template<class IndexType, class Function> void forEach(IndexType endIdx, Function&& func) {
    #ifdef MAGNUM_FLUIDSIMULATION3D_EXAMPLE_USE_MULTITHREADING
    #ifdef MAGNUM_FLUIDSIMULATION3D_EXAMPLE_USE_TBB
    tbb::parallel_for(tbb::blocked_range<IndexType>(IndexType(0), endIdx),
        [&](const tbb::blocked_range<IndexType>& r) {
            for(IndexType i = r.begin(), iEnd = r.end(); i < iEnd; ++i) {
                func(i);
            }
        });
    #else
    ThreadPool::getUniqueInstance().parallel_for(endIdx, std::forward<Function>(func));
    #endif
    #else
    for(IndexType idx = 0; idx < endIdx; ++idx) {
        func(idx);
    }
    #endif
}

}}}

#endif
