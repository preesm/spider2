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

/* === Includes === */

#include <api/spider.h>
#include <memory/Stack.h>
#include <memory/alloc.h>
#include <common/Logger.h>
#include <archi/Platform.h>
#include <graphs/pisdf/Graph.h>

/* === Static variable(s) definition === */

static bool startFlag = false;

/* === Static function(s) === */

//static std::vector<spider::pisdf::Refinement *> specialActorRefinements() {
//    std::vector<spider::pisdf::Refinement *> specialRefinements;
//    specialRefinements.reserve(spider::pisdf::SPECIAL_ACTOR_COUNT);
//
//    /* == Create the special actor refinements == */
//    auto *forkRefinement = spider::api::createRefinement("fork", spider::pisdf::fork, 0, 0, StackID::GENERAL);
//    spider::api::registerRefinement(forkRefinement);
//    specialRefinements.push_back(forkRefinement);
//
//    auto *joinRefinement = spider::api::createRefinement("join", spider::pisdf::join, 0, 0, StackID::GENERAL);
//    spider::api::registerRefinement(joinRefinement);
//    specialRefinements.emplace_back(joinRefinement);
//
//    auto *headRefinement = spider::api::createRefinement("head", spider::pisdf::head, 0, 0, StackID::GENERAL);
//    spider::api::registerRefinement(headRefinement);
//    specialRefinements.emplace_back(headRefinement);
//
//    auto *tailRefinement = spider::api::createRefinement("tail", spider::pisdf::tail, 0, 0, StackID::GENERAL);
//    spider::api::registerRefinement(tailRefinement);
//    specialRefinements.emplace_back(tailRefinement);
//
//    auto *duplicateRefinement = spider::api::createRefinement("duplicate", spider::pisdf::duplicate, 0, 0,
//                                                              StackID::GENERAL);
//    spider::api::registerRefinement(duplicateRefinement);
//    specialRefinements.emplace_back(duplicateRefinement);
//
//    auto *repeatRefinement = spider::api::createRefinement("repeat", spider::pisdf::repeat, 0, 0, StackID::GENERAL);
//    spider::api::registerRefinement(repeatRefinement);
//    specialRefinements.emplace_back(repeatRefinement);
//
//    auto *initRefinement = spider::api::createRefinement("init", spider::pisdf::init, 0, 0, StackID::GENERAL);
//    spider::api::registerRefinement(initRefinement);
//    specialRefinements.emplace_back(initRefinement);
//
//    auto *endRefinement = spider::api::createRefinement("end", spider::pisdf::end, 0, 0, StackID::GENERAL);
//    spider::api::registerRefinement(endRefinement);
//    specialRefinements.emplace_back(endRefinement);
//
//    return specialRefinements;
//}

/* === Function(s) definition === */

void spider::api::setStackAllocatorPolicy(StackID stackId,
                                          AllocatorPolicy policy,
                                          size_t alignment,
                                          size_t size,
                                          void *externBuffer) {
    auto *stack = stackArray()[static_cast<uint64_t >(stackId)];
    switch (policy) {
        case AllocatorPolicy::FREELIST_FIND_FIRST:
            stack->setPolicy(new FreeListAllocatorPolicy(size, externBuffer, FreeListPolicy::FIND_FIRST, alignment));
            break;
        case AllocatorPolicy::FREELIST_FIND_BEST:
            stack->setPolicy(new FreeListAllocatorPolicy(size, externBuffer, FreeListPolicy::FIND_BEST, alignment));
            break;
        case AllocatorPolicy::GENERIC:
            stack->setPolicy(new GenericAllocatorPolicy(alignment));
            break;
        case AllocatorPolicy::LINEAR_STATIC:
            stack->setPolicy(new LinearStaticAllocator(size, externBuffer, alignment));
            break;
    }
}

void spider::start() {
    if (startFlag) {
        throwSpiderException("spider::start() function should be called only once.");
    }

    /* == Initialize stacks == */
    auto it = EnumIterator<StackID>{ }.begin();
    for (auto &stack : stackArray()) {
        stack = new Stack(*(it++));
    }

    /* == Init the Logger and enable the GENERAL Logger == */
    log::enable<LOG_GENERAL>();

    /* == Enable the config flag == */
    startFlag = true;
}

void spider::quit() {
    /* == Destroy the spider::pisdf::Graph == */
    destroy(spider::pisdfGraph());

    /* == Destroy the Platform == */
    destroy(spider::platform());

    /* == Clear the stacks == */
    uint64_t totalUsage = 0;
    uint64_t totalAverage = 0;
    uint64_t totalPeak = 0;
    for (auto &stack : stackArray()) {
        if (stack) {
            totalUsage += stack->usage();
            totalAverage += stack->average();
            totalPeak += stack->peak();
        }
        delete stack;
    }
    Stack::print("Total", totalPeak, totalAverage, 1, totalUsage);

    /* == Reset start flag == */
    startFlag = false;
}