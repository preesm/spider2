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
#ifndef SPIDER2_SCHEDSTATSEXPORTER_H
#define SPIDER2_SCHEDSTATSEXPORTER_H

/* === Include(s) === */

#include <common/Exporter.h>
#include <common/Exception.h>

namespace spider {

    /* === Forward declaration(s) === */

    namespace sched {
        class Schedule;
    }

    /* === Class definition === */

    class SchedStatsExporter final : public Exporter {
    public:

        explicit SchedStatsExporter(const sched::Schedule *schedule) : Exporter(), schedule_{ schedule } {
            if (!schedule) {
                throwSpiderException("StatsExporter can not have nullptr for schedule.");
            }
        }

        ~SchedStatsExporter() override = default;

        /* === Method(s) === */

        /**
         * @brief Export Schedule statistics in ./stats.txt
         */
        void print() const override;

        void printFromFile(FILE *file) const override;

    private:
        const sched::Schedule *schedule_ = nullptr;

        /* === Private method(s) === */
    };

    /* === Inline method(s) === */
}

#endif //SPIDER2_SCHEDSTATSEXPORTER_H
