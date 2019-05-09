/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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

#include <common/memory/Allocator.h>

#include <common/memory/static-allocators/FreeListStaticAllocator.h>
#include <common/memory/static-allocators/LIFOStaticAllocator.h>
#include <common/memory/static-allocators/LinearStaticAllocator.h>
#include <common/memory/dynamic-allocators/FreeListAllocator.h>
#include <common/memory/dynamic-allocators/GenericAllocator.h>

void Allocator::init(SpiderStack stack, SpiderStackConfig cfg) {

}

template<typename T, class... Args>
void Allocator::construct(T *ptr, Args &&... args) {
    if (ptr) {
        new((void *) ptr) T(std::forward<Args>(args)...);
    }
}

template<typename T>
void Allocator::destroy(T *ptr) {
    if (ptr) {
        ptr->~T();
    }
}

template<typename T>
T *Allocator::allocate(SpiderStack stack, std::uint64_t size) {
    /* 0. Allocate buffer with (size + 1) to store stack identifier */
    size = size * sizeof(T);
    char *buffer = nullptr;
    switch (stack) {
        case SpiderStack::PISDF_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
            break;
        case SpiderStack::ARCHI_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
            break;
        case SpiderStack::PLATFORM_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
            break;
        case SpiderStack::TRANSFO_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
            break;
        case SpiderStack::SCHEDULE_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
            break;
        case SpiderStack::SRDAG_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
            break;
        case SpiderStack::LRT_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
            break;
        case SpiderStack::NEW_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
            break;
        default:
            break;
    }
    /* 1. Return allocated buffer */
    if (buffer) {
        ((std::uint64_t *) (buffer))[0] = (std::uint64_t) stack;
        buffer = buffer + sizeof(std::uint64_t);
    }
    return (T *) buffer;
}

void Allocator::deallocate(void *ptr) {
    if (ptr) {
        /* 0. Retrieve stack id */
        auto *originalPtr = ((char *) ptr - sizeof(std::uint64_t));
        std::uint64_t stackId = ((std::uint64_t *) (originalPtr))[0];
        /* 1. Deallocate the pointer */
    }
}
