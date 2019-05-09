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

static AbstractAllocator *newAllocator = nullptr;
static AbstractAllocator *lrtAllocator = nullptr;
static AbstractAllocator *pisdfAllocator = nullptr;
static AbstractAllocator *srdagAllocator = nullptr;
static AbstractAllocator *archiAllocator = nullptr;
static AbstractAllocator *schedAllocator = nullptr;
static AbstractAllocator *transfoAllocator = nullptr;

AbstractAllocator *&Allocator::getAllocator(SpiderStack stack) {
    switch (stack) {
        case SpiderStack::PISDF_STACK:
            return pisdfAllocator;
        case SpiderStack::ARCHI_STACK:
            return archiAllocator;
        case SpiderStack::TRANSFO_STACK:
            return transfoAllocator;
        case SpiderStack::LRT_STACK:
            return lrtAllocator;
        case SpiderStack::NEW_STACK:
            return newAllocator;
        case SpiderStack::SRDAG_STACK:
            return srdagAllocator;
        case SpiderStack::SCHEDULE_STACK:
            return schedAllocator;
        default:
            throwSpiderException("Invalid stack id: %d", stack);
    }
}

void Allocator::init(SpiderStack stack, SpiderStackConfig cfg) {
    auto *&allocator = getAllocator(stack);
    if (!allocator) {
        switch (cfg.allocatorType) {
            case SpiderAllocatorType::FREELIST:
                allocator = new FreeListAllocator(cfg.name, cfg.size, cfg.policy, cfg.alignment);
                break;
            case SpiderAllocatorType::GENERIC:
                allocator = new GenericAllocator(cfg.name, cfg.alignment);
                break;
            case SpiderAllocatorType::FREELIST_STATIC:
                allocator = new FreeListStaticAllocator(cfg.name, cfg.size, cfg.policy, cfg.alignment);
                break;
            case SpiderAllocatorType::LIFO_STATIC:
                allocator = new LIFOStaticAllocator(cfg.name, cfg.size);
                break;
            case SpiderAllocatorType::LINEAR_STATIC:
                allocator = new LinearStaticAllocator(cfg.name, cfg.size, cfg.alignment);
                break;
            default:
                throwSpiderException("Unhandled allocator type: %d", cfg.allocatorType);
        }
    }
}

void Allocator::finalize() {
    delete newAllocator;
    delete lrtAllocator;
    delete pisdfAllocator;
    delete srdagAllocator;
    delete archiAllocator;
    delete schedAllocator;
    delete transfoAllocator;
}


void Allocator::deallocate(void *ptr) {
    if (ptr) {
        /* 0. Retrieve stack id */
        auto *originalPtr = ((char *) ptr - sizeof(std::uint64_t));
        auto stackId = static_cast<SpiderStack>(((std::uint64_t *) (originalPtr))[0]);
        /* 1. Deallocate the pointer */
        auto *&allocator = getAllocator(stackId);
        allocator->dealloc(ptr);
    }
}
