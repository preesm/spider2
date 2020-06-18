/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
 * applications. The Preesm tool may be used to design PiSDF applications.
 *
 * This software is governed by the CeCILL  license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */
#ifndef SPIDER2_SEMAPHORE_H
#define SPIDER2_SEMAPHORE_H

/**
 *
 * Custom semaphore using std::condition_variable and std::mutex
 *
 * The source code comes from https://stackoverflow.com/questions/4792449/c0x-has-no-semaphores-how-to-synchronize-threads
 *
 **/

/* === Includes === */

#include <cstdint>
#include <condition_variable>
#include <mutex>

namespace spider {

    template<typename Mutex, typename CondVar>
    class basic_semaphore {
    public:
        using native_handle_type = typename CondVar::native_handle_type;

        explicit inline basic_semaphore(size_t count = 0);

        basic_semaphore(const basic_semaphore &) = delete;

        basic_semaphore(basic_semaphore &&) = delete;

        basic_semaphore &operator=(const basic_semaphore &) = delete;

        basic_semaphore &operator=(basic_semaphore &&) = delete;

        inline void notify();

        inline void wait();

        inline bool try_wait();

        template<class Rep, class Period>
        inline bool wait_for(const std::chrono::duration<Rep, Period> &d);

        template<class Clock, class Duration>
        inline bool wait_until(const std::chrono::time_point<Clock, Duration> &t);

        inline native_handle_type native_handle();

    private:
        Mutex mMutex;
        CondVar mCv;
        size_t mCount;
    };

    using semaphore = basic_semaphore<std::mutex, std::condition_variable>;

    template<typename Mutex, typename CondVar>
    basic_semaphore<Mutex, CondVar>::basic_semaphore(size_t count) : mCount{ count } { }

    template<typename Mutex, typename CondVar>
    void basic_semaphore<Mutex, CondVar>::notify() {
        std::lock_guard<Mutex> lock{ mMutex };
        ++mCount;
        mCv.notify_one();
    }

    template<typename Mutex, typename CondVar>
    void basic_semaphore<Mutex, CondVar>::wait() {
        std::unique_lock<Mutex> lock{ mMutex };
        mCv.wait(lock, [&] { return mCount > 0; });
        --mCount;
    }

    template<typename Mutex, typename CondVar>
    bool basic_semaphore<Mutex, CondVar>::try_wait() {
        std::lock_guard<Mutex> lock{ mMutex };
        if (mCount > 0) {
            --mCount;
            return true;
        }
        return false;
    }

    template<typename Mutex, typename CondVar>
    template<class Rep, class Period>
    bool basic_semaphore<Mutex, CondVar>::wait_for(const std::chrono::duration<Rep, Period> &d) {
        std::unique_lock<Mutex> lock{ mMutex };
        auto finished = mCv.wait_for(lock, d, [&] { return mCount > 0; });
        if (finished) {
            --mCount;
        }
        return finished;
    }

    template<typename Mutex, typename CondVar>
    template<class Clock, class Duration>
    bool basic_semaphore<Mutex, CondVar>::wait_until(const std::chrono::time_point<Clock, Duration> &t) {
        std::unique_lock<Mutex> lock{ mMutex };
        auto finished = mCv.wait_until(lock, t, [&] { return mCount > 0; });
        if (finished) {
            --mCount;
        }
        return finished;
    }

    template<typename Mutex, typename CondVar>
    typename basic_semaphore<Mutex, CondVar>::native_handle_type basic_semaphore<Mutex, CondVar>::native_handle() {
        return mCv.native_handle();
    }
}

#endif //SPIDER2_SEMAPHORE_H
