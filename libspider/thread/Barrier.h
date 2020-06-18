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
#ifndef SPIDER2_BARRIER_H
#define SPIDER2_BARRIER_H

/* === Include(s) === */

#include <cstdint>
#include <condition_variable>
#include <mutex>

namespace spider {
    /* === Class definition === */

    template<typename Mutex, typename CondVar>
    class basic_barrier {
    public:
        using native_handle_type = typename CondVar::native_handle_type;

        explicit inline basic_barrier(size_t count = 0) noexcept;

        basic_barrier(const basic_barrier &) = delete;

        basic_barrier(basic_barrier &&) = delete;

        basic_barrier &operator=(const basic_barrier &) = delete;

        basic_barrier &operator=(basic_barrier &&) = delete;

        ~basic_barrier() = default;

        /* === Method(s) === */

        inline void wait();

        /* === Getter(s) === */

        inline native_handle_type native_handle() {
            return condVar_.native_handle();
        }

        /* === Setter(s) === */

    private:
        Mutex mutex_;
        CondVar condVar_;
        size_t count_reset_value_;
        size_t count_;
        size_t done_;
    };

    using barrier = basic_barrier<std::mutex, std::condition_variable>;

    /* === Inline method(s) === */

    template<typename Mutex, typename CondVar>
    basic_barrier<Mutex, CondVar>::basic_barrier(size_t count) noexcept : count_reset_value_{ count },
                                                                          count_{ count },
                                                                          done_{ 0 } { }

    template<typename Mutex, typename CondVar>
    void basic_barrier<Mutex, CondVar>::wait() {
        std::unique_lock<Mutex> lock{ mutex_ };
        const auto &done = done_;
        if (!(--count_)) {
            done_++;
            count_ = count_reset_value_;
            condVar_.notify_all();
        } else {
            condVar_.wait(lock, [done, this] { return done_ != done; });
        }
    }
}

#endif //SPIDER2_BARRIER_H
