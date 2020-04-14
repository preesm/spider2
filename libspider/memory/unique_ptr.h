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
#ifndef SPIDER2_UNIQUE_PTR_H
#define SPIDER2_UNIQUE_PTR_H

/* === Include(s) === */

#include <utility>
#include <api/global-api.h>
#include <memory/memory.h>

namespace spider {

    /* === Class definition === */

    template<class T>
    class unique_ptr {
        template<class U>
        struct ptr_t {
            using type = U *;
        };
    public:
        using pointer = typename ptr_t<T>::type;
        using element_type  = T;

        constexpr unique_ptr() noexcept : data_{ nullptr } { }

        explicit unique_ptr(T *value) : data_{ value } { }

        unique_ptr(unique_ptr &&rhs) noexcept : data_{ rhs.release() } { }

        template<typename U, typename = typename std::_Require<
                std::is_convertible<typename unique_ptr<U>::pointer, pointer>>>
        explicit unique_ptr(U *value) : data_{ value } { }

        template<typename U, typename = typename std::_Require<
                std::is_convertible<typename unique_ptr<U>::pointer, pointer>>>
        unique_ptr(unique_ptr<U> &&rhs) noexcept : data_{ rhs.release() } { }

        ~unique_ptr() {
            destroy(data_);
        };

        /* === Remove copy possibilities === */

        unique_ptr(const unique_ptr &) = delete;

        unique_ptr &operator=(const unique_ptr &) = delete;

        /* === Method(s) === */

        T *get() const noexcept { return data_; }

        T *release() {
            T *tmp = data_;
            data_ = nullptr;
            return tmp;
        }

        void reset(T *ptr = nullptr) {
            std::swap(data_, ptr);
            destroy(ptr);
        }

        /* === Operators === */

        unique_ptr &operator=(unique_ptr &&rhs) noexcept {
            reset(rhs.release());
            return *this;
        };

        T &operator*() const {
            assert(data_ != nullptr);
            return *get();
        };

        T *operator->() const noexcept { return get(); };

        explicit operator bool() const noexcept { return data_; }

    private:
        T *data_ = nullptr;
    };

    template<class T, StackID stack = StackID::GENERAL, class ...Args>
    unique_ptr<T> make_unique(Args &&... args) {
        return spider::unique_ptr<T>(make<T, stack>(std::forward<Args>(args)...));
    }

    template<class T, class ...Args>
    unique_ptr<T> make_unique(StackID stack, Args &&... args) {
        return unique_ptr<T>(make<T>(stack, std::forward<Args>(args)...));
    }

    template<class T>
    spider::unique_ptr<T> make_unique(T *value) {
        return unique_ptr<T>(value);
    }
}
#endif //SPIDER2_UNIQUE_PTR_H
