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
/* === Include(s) === */

#include <new>
#include <memory/memory.h>

/* === Function(s) definition === */

void *spider::allocate(std::unique_ptr<Stack> &stack, size_t size, size_t n) {
    if (!n) {
        return nullptr;
    }
    auto buffer = reinterpret_cast<uintptr_t>(stack->allocate(n * size + sizeof(uint64_t)));

    /* == Return allocated buffer == */
    if (buffer) {
        reinterpret_cast<uint64_t *>(buffer)[0] = static_cast<uint64_t>(stack->id());
        buffer = buffer + sizeof(uint64_t);
        return reinterpret_cast<void *>(buffer);
    }
    return nullptr;
}

void spider::deallocate(void *ptr) {
    if (!ptr) {
        return;
    }
    /* == Retrieve stack id == */
    auto *originalPtr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) - sizeof(uint64_t));
    auto stackId = static_cast<StackID>(reinterpret_cast<uint64_t *>(originalPtr)[0]);

    /* == Deallocate the pointer == */
    auto &stack = stackArray()[static_cast<size_t>(stackId)];
    stack->deallocate(originalPtr);
}

/* === Overload operator new / delete === */

void *operator new(std::size_t size) {
    return std::malloc(size);
}

void *operator new[](std::size_t size) {
    return std::malloc(size);
}

void operator delete(void *ptr) noexcept {
    std::free(ptr);
}

void operator delete[](void *ptr) noexcept {
    std::free(ptr);
}

void operator delete(void *ptr, std::size_t) noexcept {
    std::free(ptr);
}

void operator delete[](void *ptr, std::size_t) noexcept {
    std::free(ptr);
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
    return std::malloc(size);
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
    return std::malloc(size);
}

void operator delete(void *ptr, const std::nothrow_t &) noexcept {
    std::free(ptr);
}

void operator delete[](void *ptr, const std::nothrow_t &) noexcept {
    std::free(ptr);
}
