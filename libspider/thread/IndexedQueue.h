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
#ifndef SPIDER2_INDEXEDQUEUE_H
#define SPIDER2_INDEXEDQUEUE_H

/* === Include(s) === */

#include <mutex>
#include <containers/queue.h>
#include <containers/vector.h>

namespace spider {

    /* === Class definition === */

    /**
     * @brief Thread-safe Indexed Queue (i.e message retrieval is through ix)
     * @tparam T Type of the message.
     */
    template<class T>
    class IndexedQueue {
    public:
        IndexedQueue() = default;

        /**
         * @brief Copy constructor. mutex state of the queue will NOT be copied.
         */
        IndexedQueue(const IndexedQueue<T> &other) : IndexedQueue() {
            queue_ = other.queue_;
            freeIndexQueue_ = other.freeIndexQueue_;
        }

        /**
         * @brief Move constructor. mutex state of the queue will NOT be moved.
         */
        IndexedQueue(IndexedQueue<T> &&other) noexcept : IndexedQueue() {
            queue_.swap(other.queue_);
            freeIndexQueue_.swap(other.freeIndexQueue_);
        }

        IndexedQueue &operator=(const IndexedQueue<T> &) = delete;

        IndexedQueue &operator=(IndexedQueue<T> &&) noexcept = delete;

        ~IndexedQueue() = default;

        /* === Method(s) === */

        /**
         * @brief Push data through copy ctor into the queue.
         * @param value  Pointer to the load to push.
         * @return Index of the item pushed in the queue (for latter retrieval).
         */
        inline size_t push(T value) {
            std::lock_guard<std::mutex> lock{ mutex_ };
            auto &&index = getFreeIndex();
            if (index == SIZE_MAX) {
                queue_.emplace_back(std::move(value));
                return queue_.size() - 1;
            }
            queue_[index] = std::move(value);
            return index;
        }

        /**
         * @brief Pop element and move it to (*load) using std::move (if move assignment is available).
         * @param load  Pointer to the load to be filled.
         * @param ix    Ix of the item to fetch in the queue
         * @return true on success, false if ix < 0 || ix >= queue_.size().
         */
        inline bool pop(T &load, size_t ix) {
            std::lock_guard<std::mutex> lock{ mutex_ };
            /* == std::vector are thread-safe in read-only only if no other thread is writing == */
            load = std::move(queue_.at(ix)); /* = use move assignment if available = */

            /* == Push index as available one == */
            freeIndexQueue_.push(ix);
            return true;
        }

        /* === Getter(s) === */

        /* === Setter(s) === */

    private:
        spider::queue<size_t> freeIndexQueue_; /* = Keeping track of available space in vector = */
        spider::vector<T> queue_;              /* = Actual queue = */
        std::mutex mutex_;

        /* === Private method(s) === */

        /**
         * @brief Get an index of free location in the queue (if any).
         * @return index of available location, -1 if none
         */
        inline size_t getFreeIndex() {
            if (!freeIndexQueue_.empty()) {
                auto &&front = freeIndexQueue_.front();
                freeIndexQueue_.pop();
                return front;
            }
            return SIZE_MAX;
        }
    };
}

#endif //SPIDER2_INDEXEDQUEUE_H
