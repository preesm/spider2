/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
 *
 * Spider is a dataflow based runtime used to execute dynamic PiSDF
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
#ifndef SPIDER2_QUEUE_H
#define SPIDER2_QUEUE_H

/* === Include(s) === */

#include <containers/containers.h>
#include <thread/Semaphore.h>
#include <mutex>

namespace spider {

    /* === Class definition === */

    /**
     * @brief Thread-safe implementation using std::mutex and spider::semaphore as synchronization mechanism.
     */
    template<class T>
    class Queue {
    public:

        Queue() = default;

        Queue(const Queue<T> &) = delete;

        Queue(Queue<T> &&) noexcept = delete;

        ~Queue() = default;

        Queue &operator=(const Queue<T> &) = delete;

        Queue &operator=(Queue<T> &&) noexcept = delete;

        /* === Method(s) === */

        /**
         * @brief Clear the underlaying queue.
         */
        inline void clear() {
            std::lock_guard<std::mutex> lock{ mutex_ };
            while (!queue_.empty()) { queue_.pop(); }
        }

        /**
         * @brief Pop an element from the queue.
         * @remark Call to pop will block until an element is pushed in the queue.
         * @param data Pointer to be filled with front element of the queue.
         * @return true if success, nothing it would just hang in spider::basic_semaphore::wait().
         */
        inline bool pop(T &data) {
            sem_.wait();
            std::lock_guard<std::mutex> lock{ mutex_ };
            data = std::move(static_cast<T>(queue_.front()));
            queue_.pop();
            return true;
        }

        /**
         * @brief Pop an element from the queue.
         * @remark If queue is empty, function calls return immediately with value false.
         * @param data Pointer to be filled with front element of the queue.
         * @return true if success, false if try_wait() failed.
         */
        inline bool try_pop(T &data) {
            if (!sem_.try_wait()) {
                return false;
            }
            std::lock_guard<std::mutex> lock{ mutex_ };
            data = std::move(static_cast<T>(queue_.front()));
            queue_.pop();
            return true;
        }


        /**
         * @brief Push data into the queue.
         * @param data Pointer to the data to be pushed (call copy ctor of T)
         */
        inline void push(T data) {
            std::lock_guard<std::mutex> lock{ mutex_ };
            queue_.push(std::move(data));
            sem_.notify();
        }

    private:
        std::mutex mutex_;
        spider::semaphore sem_;
        spider::queue<T> queue_;
    };
}

#endif //SPIDER2_QUEUE_H
