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
#ifndef SPIDER2_SCHEDSVGGANTTEXPORTER_H
#define SPIDER2_SCHEDSVGGANTTEXPORTER_H

/* === Include(s) === */

#include <common/Exporter.h>
#include <common/Types.h>
#include <containers/vector.h>
#include <scheduling/schedule/exporter/GanttTask.h>


namespace spider {

    /* === Forward declaration(s) === */

    class ScheduleLegacy;

    class ScheduleTask;

    namespace pisdf {
        class Graph;
    }

    /* === Class definition === */

    class SchedSVGGanttExporter final : public Exporter {
    public:

        explicit SchedSVGGanttExporter(const ScheduleLegacy *schedule);

        ~SchedSVGGanttExporter() override = default;

        /* === Method(s) === */

        /**
         * @brief Print the graph to default file path.
         * @remark default path: ./gantt.svg
         */
        void print() const override;

        void printFromFile(FILE *file) const override;

        void printFromTasks(const vector<GanttTask> &taskVector, const std::string &path = "./gantt.xml");

    private:
        const ScheduleLegacy *schedule_ = nullptr;
        double widthMin_ = 0;
        double widthMax_ = 0;
        double alpha_ = 0.;
        u64 width_ = 0;
        u64 height_ = 0;
        u64 makespanWidth_ = 0;
        u32 offsetX_ = 0;

        /* === Private method(s) === */

        u32 computeRealXOffset() const;

        u64 computeWidth(u64 time) const;

        void pePrinter(FILE *file) const;

        void headerPrinter(FILE *file) const;

        void axisPrinter(FILE *file) const;

        void taskPrinter(FILE *file, const ScheduleTask *task) const;
    };

    /* === Inline method(s) === */
}
#endif //SPIDER2_SCHEDSVGGANTTEXPORTER_H
