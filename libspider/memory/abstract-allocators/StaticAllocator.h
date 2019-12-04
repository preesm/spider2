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
#ifndef SPIDER2_STATICALLOCATOR_H
#define SPIDER2_STATICALLOCATOR_H

/* === Includes === */

#include "AbstractAllocator.h"

/* === Class definition === */

class StaticAllocator : public AbstractAllocator {
public:
    void *allocate(size_t size) override = 0;

    void deallocate(void *ptr) override = 0;

    virtual void reset() noexcept = 0;

protected:
    size_t totalSize_;
    bool externalBase_;
    void *startPtr_;

    StaticAllocator(std::string name, size_t totalSize, size_t alignment = 0) :
            AbstractAllocator(std::move(name), alignment),
            totalSize_{ totalSize },
            startPtr_{ nullptr } {
        if (!totalSize) {
            throwSpiderException("Allocator size should be >= 0.\n");
        }
        startPtr_ = std::malloc(totalSize_);
        externalBase_ = false;
    }

    StaticAllocator(std::string name, size_t totalSize, void *externalBase, size_t alignment = 0) :
            AbstractAllocator(std::move(name), alignment),
            totalSize_{ totalSize },
            startPtr_{ nullptr } {
        if (!totalSize) {
            throwSpiderException("Allocator size should be >= 0.\n");
        }
        if (!externalBase) {
            throwSpiderException("External base address should not be null.");
        }
        startPtr_ = externalBase;
        externalBase_ = true;
    }

    ~StaticAllocator() noexcept override {
        if (!externalBase_) {
            std::free(startPtr_);
        }
    }

    inline void checkPointerAddress(void *ptr) const {
        const auto &uintPtr = reinterpret_cast<uintptr_t >(ptr);
        if (uintPtr < reinterpret_cast<uintptr_t>(startPtr_)) {
            throwSpiderException("Trying to deallocate unallocated memory block.");
        }

        if (uintPtr > (reinterpret_cast<uintptr_t>(startPtr_) + totalSize_)) {
            throwSpiderException("Trying to deallocate memory block out of memory space.");
        }
    }
};

#endif //SPIDER2_STATICALLOCATOR_H