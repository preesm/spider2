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
#include <memory/Allocator.h>
#include <common/Logger.h>
#include <archi/Platform.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/common/Refinement.h>

/* === Static variable(s) definition === */

static bool &startFlag() {
    static bool startFlag = false;
    return startFlag;
}

/* === Static function(s) === */


static std::vector<Spider::PiSDF::Refinement *> specialActorRefinements() {
    std::vector<Spider::PiSDF::Refinement *> specialRefinements;
    specialRefinements.reserve(specialActorCount);

    /* == Create the special actor refinements == */
    auto *forkRefinement = Spider::API::createRefinement("fork", Spider::PiSDF::fork, 0, 0, StackID::GENERAL);
    Spider::API::registerRefinement(forkRefinement);
    specialRefinements.push_back(forkRefinement);

    auto *joinRefinement = Spider::API::createRefinement("join", Spider::PiSDF::join, 0, 0, StackID::GENERAL);
    Spider::API::registerRefinement(joinRefinement);
    specialRefinements.emplace_back(joinRefinement);

    auto *headRefinement = Spider::API::createRefinement("head", Spider::PiSDF::head, 0, 0, StackID::GENERAL);
    Spider::API::registerRefinement(headRefinement);
    specialRefinements.emplace_back(headRefinement);

    auto *tailRefinement = Spider::API::createRefinement("tail", Spider::PiSDF::tail, 0, 0, StackID::GENERAL);
    Spider::API::registerRefinement(tailRefinement);
    specialRefinements.emplace_back(tailRefinement);

    auto *duplicateRefinement = Spider::API::createRefinement("duplicate", Spider::PiSDF::duplicate, 0, 0,
                                                              StackID::GENERAL);
    Spider::API::registerRefinement(duplicateRefinement);
    specialRefinements.emplace_back(duplicateRefinement);

    auto *repeatRefinement = Spider::API::createRefinement("repeat", Spider::PiSDF::repeat, 0, 0,
                                                           StackID::GENERAL);
    Spider::API::registerRefinement(repeatRefinement);
    specialRefinements.emplace_back(repeatRefinement);

    auto *initRefinement = Spider::API::createRefinement("init", Spider::PiSDF::init, 0, 0, StackID::GENERAL);
    Spider::API::registerRefinement(initRefinement);
    specialRefinements.emplace_back(initRefinement);

    auto *endRefinement = Spider::API::createRefinement("end", Spider::PiSDF::end, 0, 0, StackID::GENERAL);
    Spider::API::registerRefinement(endRefinement);
    specialRefinements.emplace_back(endRefinement);

    return specialRefinements;
}

/* === Function(s) definition === */

void Spider::API::initStack(StackID stackId,
                            const std::string &name,
                            AllocatorType type,
                            std::uint64_t size,
                            char *baseAddr,
                            std::uint64_t alignment) {
    if (!startFlag()) {
        throwSpiderException("Method Spider::start() should be called first.");
    }

    /* == Create the corresponding AllocatorConfig == */
    auto cfg = AllocatorConfig{ name, type, size, alignment, FreeListPolicy::FIND_FIRST, baseAddr };

    /* == Do the actual init of the allocator == */
    Spider::initAllocator(stackId, cfg);
}

void Spider::API::initStack(StackID stackId,
                            const std::string &name,
                            AllocatorType type,
                            std::uint64_t size,
                            FreeListPolicy policy,
                            char *baseAddr,
                            std::uint64_t alignment) {

    if (!startFlag()) {
        throwSpiderException("Method Spider::start() should be called first.");
    }

    /* == Create the corresponding AllocatorConfig == */
    auto cfg = AllocatorConfig{ name, type, size, alignment, policy, baseAddr };

    /* == Do the actual init of the allocator == */
    Spider::initAllocator(stackId, cfg);
}


void Spider::start() {
    if (startFlag()) {
        throwSpiderException("Spider::start() function should be called only once.");
    }
    /* == General stack initialization == */
    auto cfg = AllocatorConfig{ "general-allocator",
                                AllocatorType::FREELIST,
                                16392,
                                sizeof(std::uint64_t),
                                FreeListPolicy::FIND_FIRST,
                                nullptr };
    Spider::initAllocator(StackID::GENERAL, cfg);

    /* == Init the Logger and enable the GENERAL Logger == */
    Logger::enable<LOG_GENERAL>();

    /* == Init the special actor refinements == */
    Spider::refinementsRegister() = specialActorRefinements();

    /* == Enable the config flag == */
    startFlag() = true;
}

void Spider::quit() {
    /* == Destroy the PiSDFGraph == */
    {
        auto *&applicationGraph = Spider::pisdfGraph();
        if (applicationGraph) {
            Spider::destroy(applicationGraph);
            Spider::deallocate(applicationGraph);
        }
    }

    /* == Destroy the Platform == */
    {
        auto *&platform = Spider::platform();
        if (platform) {
            Spider::destroy(platform);
            Spider::deallocate(platform);
        }
    }

    /* == Destroy the refinement(s) == */
    {
        const auto &refinementVector = Spider::refinementsRegister();
        for (auto &refinement : refinementVector) {
            Spider::destroy(refinement);
            Spider::deallocate(refinement);
        }
    }


    /* == Clear the stacks == */
    Spider::finalizeAllocators();
}