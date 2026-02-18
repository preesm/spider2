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

#include <scheduling/mapper/RoundRobinMapper.h>
#include <scheduling/schedule/Schedule.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::RoundRobinMapper::RoundRobinMapper() : Mapper() {
    currentPeIx_ = spider::make_unique(make_n<size_t, StackID::SCHEDULE>(archi::platform()->clusterCount(), 0));
}

/* === Private method(s) implementation === */

const spider::PE *spider::sched::RoundRobinMapper::findPE(const Cluster *cluster,
                                                          const Stats &,
                                                          const Task *task,
                                                          ufast64) const {
    const auto clusterPeCount = cluster->PECount();
    if (!clusterPeCount) {
        return nullptr;
    }
    const auto clusterIx = cluster->ix();
    const auto *pe = cluster->peArray()[currentPeIx_[clusterIx]];
    auto count = size_t{ 0 };
    while ((!pe->enabled() || !task->isMappableOnPE(pe)) && count < clusterPeCount) {
        currentPeIx_[clusterIx] = (currentPeIx_[clusterIx] + 1u) % clusterPeCount;
        pe = cluster->peArray()[currentPeIx_[clusterIx]];
        count++;
    }
    if (!pe->enabled() || !task->isMappableOnPE(pe)) {
        return nullptr;
    }
    currentPeIx_[cluster->ix()] = (currentPeIx_[clusterIx] + 1u) % clusterPeCount;
    return pe;
}
