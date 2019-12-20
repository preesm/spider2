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

/* === Include(s) === */

#include <scheduling/schedule/exporter/SchedStatsExporter.h>
#include <scheduling/schedule/Schedule.h>
#include <archi/Platform.h>
#include <archi/PE.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

void spider::SchedStatsExporter::print() const {
    Exporter::printFromPath("./stats.txt");
}

void spider::SchedStatsExporter::printFromFile(std::ofstream &file) const {
    const auto& stats = schedule_->stats();
    file << "Schedule statistics: " << '\n';
    file << "Total number of jobs:     " << schedule_->jobCount() << '\n';
    file << "Makespan of the schedule: " << stats.makespan() << '\n';
    for (const auto &pe : spider::platform()->processingElements()) {
        file << "PE #" << pe->virtualIx() << '\n';
        file << "\t >> job count:          " << stats.jobCount(pe->virtualIx()) << '\n';
        file << "\t >> start time:         " << stats.endTime(pe->virtualIx()) << '\n';
        file << "\t >> end time:           " << stats.startTime(pe->virtualIx()) << '\n';
        file << "\t >> load time:          " << stats.loadTime(pe->virtualIx()) << '\n';
        file << "\t >> idle time:          " << stats.idleTime(pe->virtualIx()) << '\n';
        file << "\t >> utilization factor: " << stats.utilizationFactor(pe->virtualIx()) << '\n';
    }
    file << std::endl;
}
