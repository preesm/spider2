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
#ifndef SPIDER2_THREAD_H
#define SPIDER2_THREAD_H

/* === Include(s) === */

#include <thread>

namespace spider {

    /* === Class definition === */

    class thread final : public std::thread {
    public:

        thread() : std::thread() { };

        template<class Fct, class ...Args>
        explicit thread(Fct &&fn, Args &&... args) : std::thread(fn, std::forward<Args>(args)...) { }

        thread(const thread &) = delete;

        thread(thread &&) noexcept = default;

        ~thread() = default;

        /* === Method(s) === */

        /* === Getter(s) === */

        inline std::int32_t affinity() const;

        /* === Setter(s) === */

        /**
         * @brief Set the affinity of the thread (i.e which physical resource it runs on)
         * @note This implementation is strongly inspired by https://searchcode.com/file/92960692/Source/Core/Common/Thread.cpp
         *       and https://gist.github.com/Coneko/4234842
         * @param affinity_id Mask to use.
         * @return true if it was successful, false else
         */
        bool set_affinity(std::int32_t affinity_id);

    private:
        std::int32_t affinity_ = -1;
    };

    /* === Inline method(s) === */


    std::int32_t spider::thread::affinity() const {
        return affinity_;
    }

    namespace this_thread {

        /**
         * @brief Return current native handle automatically from the thread.
         * @return native_handle_type
         */
        std::thread::native_handle_type native_handle();

        /**
         * @brief Interface wrapper to std::this_thread::get_id function to have uniform API in spider::thread.
         * @return std::thread::id value of this_thread.
         */
        inline std::thread::id get_id() {
            return std::this_thread::get_id();
        }
    }

}

#endif //SPIDER2_THREAD_H
