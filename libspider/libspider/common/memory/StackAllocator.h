/**
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
#ifndef SPIDER_STACKALLOCATOR_H
#define SPIDER_STACKALLOCATOR_H

#include <spider/spider.h>

typedef enum {
    PISDF_STACK,
    SRDAG_STACK,
    ARCHI_STACK,
    TRANSFO_STACK,
    LRT_STACK,
    STACK_COUNT
} SpiderStack;


#define CREATE(stackId, type) new(StackMonitor::alloc(stackId, sizeof(type), true)) type
#define CREATE_MUL(stackId, size, type) new(StackMonitor::alloc(stackId, (size)*sizeof(type), true)) type[size]

#define CREATE_NA(stackId, type) new(StackMonitor::alloc(stackId, sizeof(type), false)) type
#define CREATE_MUL_NA(stackId, size, type) new(StackMonitor::alloc(stackId, (size)*sizeof(type), false)) type[size]

namespace StackAllocator {
    void initStack(SpiderStack id, SpiderStackConfig cfg);

    void clean(SpiderStack id);

    void cleanAllStack();

    template <typename T>
    T *alloc(SpiderStack id, int size, bool pageAligned = true) {
        return nullptr;
    }

    void free(SpiderStack id, void *ptr);

    void freeAll(SpiderStack id, const char *function);

    void freeAll(SpiderStack id);

    void printStackStats();
};

#endif /* SPIDER_STACKALLOCATOR_H */

