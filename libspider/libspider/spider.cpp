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

#include <spider.h>
#include <memory/allocator.h>
#include <common/Logger.h>
#include <archi/Platform.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/common/Refinement.h>

/* === Static variable(s) definition === */

/* === Static function(s) === */

static std::vector<spider::pisdf::Refinement *> specialActorRefinements() {
    std::vector<spider::pisdf::Refinement *> specialRefinements;
    specialRefinements.reserve(spider::pisdf::SPECIAL_ACTOR_COUNT);

    /* == Create the special actor refinements == */
    auto *forkRefinement = spider::api::createRefinement("fork", spider::pisdf::fork, 0, 0, StackID::GENERAL);
    spider::api::registerRefinement(forkRefinement);
    specialRefinements.push_back(forkRefinement);

    auto *joinRefinement = spider::api::createRefinement("join", spider::pisdf::join, 0, 0, StackID::GENERAL);
    spider::api::registerRefinement(joinRefinement);
    specialRefinements.emplace_back(joinRefinement);

    auto *headRefinement = spider::api::createRefinement("head", spider::pisdf::head, 0, 0, StackID::GENERAL);
    spider::api::registerRefinement(headRefinement);
    specialRefinements.emplace_back(headRefinement);

    auto *tailRefinement = spider::api::createRefinement("tail", spider::pisdf::tail, 0, 0, StackID::GENERAL);
    spider::api::registerRefinement(tailRefinement);
    specialRefinements.emplace_back(tailRefinement);

    auto *duplicateRefinement = spider::api::createRefinement("duplicate", spider::pisdf::duplicate, 0, 0,
                                                              StackID::GENERAL);
    spider::api::registerRefinement(duplicateRefinement);
    specialRefinements.emplace_back(duplicateRefinement);

    auto *repeatRefinement = spider::api::createRefinement("repeat", spider::pisdf::repeat, 0, 0, StackID::GENERAL);
    spider::api::registerRefinement(repeatRefinement);
    specialRefinements.emplace_back(repeatRefinement);

    auto *initRefinement = spider::api::createRefinement("init", spider::pisdf::init, 0, 0, StackID::GENERAL);
    spider::api::registerRefinement(initRefinement);
    specialRefinements.emplace_back(initRefinement);

    auto *endRefinement = spider::api::createRefinement("end", spider::pisdf::end, 0, 0, StackID::GENERAL);
    spider::api::registerRefinement(endRefinement);
    specialRefinements.emplace_back(endRefinement);

    return specialRefinements;
}

/* === Function(s) definition === */

void spider::api::createGenericStack(StackID stack, const std::string &name, size_t alignment) {
    createAllocator(spider::type<AllocatorType::GENERIC>{ }, stack, name, alignment);
}

void spider::api::createFreeListStack(StackID stack,
                                      const std::string &name,
                                      size_t staticBufferSize,
                                      FreeListPolicy policy,
                                      size_t alignment) {
    createAllocator(spider::type<AllocatorType::FREELIST>{ }, stack, name, staticBufferSize, policy, alignment);
}

void spider::api::createLinearStaticStack(StackID stack,
                                          const std::string &name,
                                          size_t totalSize,
                                          size_t alignment) {
    createAllocator(spider::type<AllocatorType::LINEAR_STATIC>{ }, stack, name, totalSize, alignment);
}

void spider::api::createLinearStaticStack(StackID stack,
                                          const std::string &name,
                                          size_t totalSize,
                                          void *base,
                                          size_t alignment) {
    createAllocator(spider::type<AllocatorType::LINEAR_STATIC>{ }, stack, name, totalSize, base, alignment);
}

void spider::api::createLIFOStaticStack(StackID stack, const std::string &name, size_t totalSize) {
    createAllocator(spider::type<AllocatorType::LIFO_STATIC>{ }, stack, name, totalSize);

}

void spider::api::createLIFOStaticStack(StackID stack, const std::string &name, size_t totalSize, void *base) {
    createAllocator(spider::type<AllocatorType::LIFO_STATIC>{ }, stack, name, totalSize, base);
}

void spider::start() {
    static bool start = false;
    if (start) {
        throwSpiderException("spider::start() function should be called only once.");
    }
    /* == General stack initialization == */
    spider::api::createGenericStack(StackID::GENERAL, "general-allocator");

    /* == Init the Logger and enable the GENERAL Logger == */
    log::enable<LOG_GENERAL>();

    /* == Init the special actor refinements == */
    spider::refinementsRegister() = specialActorRefinements();

    /* == Enable the config flag == */
    start = true;
}

void spider::quit() {
    /* == Destroy the PiSDFGraph == */
    spider::destroy(spider::pisdfGraph());

    /* == Destroy the Platform == */
    spider::destroy(spider::platform());

    /* == Destroy the refinement(s) == */
    const auto &refinementVector = spider::refinementsRegister();
    for (auto &refinement : refinementVector) {
        spider::destroy(refinement);
    }


    /* == Clear the stacks == */
    spider::freeAllocators();
}