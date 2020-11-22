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

#include <scheduling/mapper/BestFitMapper.h>
#include <scheduling/schedule/Schedule.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>

/* === Private method(s) implementation === */

const spider::PE *spider::sched::BestFitMapper::findPE(const Cluster *cluster,
                                                       const Stats &stats,
                                                       const Task *task,
                                                       ufast64 minStartTime) const {
    const auto *grtPE = archi::platform()->spiderGRTPE();
    const PE *foundPE = nullptr;
    auto bestFitIdleTime = UINT_FAST64_MAX;
    auto bestFitEndTime = UINT_FAST64_MAX;
    for (const auto *pe : cluster->peArray()) {
        if (!pe->enabled() || !task->isMappableOnPE(pe)) {
            continue;
        }
        /* == Add a small overhead in choosing GRT as a mapping choice to break inequality in favor of other PEs == */
        const auto readyTime = stats.endTime(pe->virtualIx()) + (pe == grtPE) * 10;
        const auto startTime = std::max(readyTime, minStartTime);
        const auto idleTime = startTime - readyTime;
        const auto endTime = startTime + task->timingOnPE(pe);
        if (endTime < bestFitEndTime) {
            foundPE = pe;
            bestFitEndTime = endTime;
            bestFitIdleTime = std::min(idleTime, bestFitIdleTime);
        } else if ((endTime == bestFitEndTime) && (idleTime < bestFitIdleTime)) {
            foundPE = pe;
            bestFitEndTime = endTime;
            bestFitIdleTime = idleTime;
        }
    }
    return foundPE;
}
