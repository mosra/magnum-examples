#ifndef Magnum_Examples_FluidSimulation3D_ThreadPool_h
#define Magnum_Examples_FluidSimulation3D_ThreadPool_h
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

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <Magnum/Math/Functions.h>

namespace Magnum { namespace Examples {

/* This is a very simple threadpool implementation, for demonstration purpose
   only. Using tbb::parallel_for from Intel TBB yields higher performance --
   see TaskScheduler.h. */
class ThreadPool {
    public:
        ThreadPool() {
            const Int maxNumThreads = Int(std::thread::hardware_concurrency());
            std::size_t nWorkers = std::size_t(maxNumThreads > 1 ? maxNumThreads - 1 : 0);

            _threadTaskReady.resize(nWorkers, 0);
            _tasks.resize(nWorkers + 1);

            for(std::size_t threadIdx = 0; threadIdx < nWorkers; ++threadIdx) {
                _workerThreads.emplace_back([threadIdx, this] {
                    for(;;) {
                        {
                            std::unique_lock<std::mutex> lock(_taskMutex);
                            _condition.wait(lock, [threadIdx, this] {
                                return _bStop || _threadTaskReady[threadIdx] == 1;
                            });
                            if(_bStop && !_threadTaskReady[threadIdx]) return;
                        }

                        _tasks[threadIdx](); /* run task */

                        /* Set task ready to 0, thus this thread will not do
                           its computation more than once */
                        _threadTaskReady[threadIdx] = 0;

                        /* Decrease the busy thread counter */
                        _numBusyThreads.fetch_add(-1);
                    }
                });
            }
        }

        ~ThreadPool() {
            {
                std::unique_lock<std::mutex> lock(_taskMutex);
                _bStop = true;
            }

            _condition.notify_all();
            for(std::thread& worker: _workerThreads) worker.join();
        }

        void parallel_for(std::size_t size, std::function<void(std::size_t)>&& func) {
            const auto nWorkers = _workerThreads.size();
            if(nWorkers > 0) {
                _numBusyThreads = Int(nWorkers);

                const std::size_t chunkSize = std::size_t(Math::ceil(Float(size)/ Float(nWorkers + 1)));
                for(std::size_t threadIdx = 0; threadIdx < nWorkers + 1; ++threadIdx) {
                    const std::size_t chunkStart = threadIdx * chunkSize;
                    const std::size_t chunkEnd = Math::min(chunkStart + chunkSize, size);

                    /* Must copy func into local lambda's variable */
                    _tasks[threadIdx] = [chunkStart, chunkEnd, func] {
                        for(uint64_t idx = chunkStart; idx < chunkEnd; ++idx) {
                            func(idx);
                        }
                    };
                }

                /* Wake up worker threads */
                {
                    std::unique_lock<std::mutex> lock(_taskMutex);
                    for(std::size_t threadIdx = 0; threadIdx < _threadTaskReady.size(); ++threadIdx)
                        _threadTaskReady[threadIdx] = 1;
                }
                _condition.notify_all();

                /* Handle last chunk in this thread */
                _tasks.back()();

                /* Wait until all worker threads finish */
                while(_numBusyThreads.load() > 0) {}

            } else for(std::size_t idx = 0; idx < size; ++idx)
                func(idx);
        }

        static ThreadPool& getUniqueInstance() {
            static ThreadPool threadPool;
            return threadPool;
        }

    private:
        std::atomic<int> _numBusyThreads{0};
        /* Do not use std::vector<bool>: it's not threadsafe */
        std::vector<int> _threadTaskReady;
        std::vector<std::thread> _workerThreads;

        std::vector<std::function<void()>> _tasks;
        std::mutex _taskMutex;
        std::condition_variable _condition;
        bool _bStop = false;
};

}}

#endif
