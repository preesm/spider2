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
#ifndef SPIDER2_SVGGANTTEXPORTER_H
#define SPIDER2_SVGGANTTEXPORTER_H

/* === Include(s) === */

#include <common/Exporter.h>

namespace spider {

    /* === Forward declaration(s) === */

    namespace sched {
        class Schedule;

        class Job;
    }

    namespace pisdf {
        class Graph;
    }

    /* === Class definition === */

    class SVGGanttExporter final : public Exporter {
    public:

        explicit SVGGanttExporter(const sched::Schedule *schedule, const pisdf::Graph *graph);

        ~SVGGanttExporter() override = default;

        /* === Method(s) === */

        /**
         * @brief Print the graph to default file path.
         * @remark default path: ./gantt.svg
         */
        void print() const override;

        void print(const std::string &path) const override;

        void print(std::ofstream &file) const override;

    private:
        const sched::Schedule *schedule_ = nullptr;
        const pisdf::Graph *graph_ = nullptr;
        uint64_t width_ = UINT32_MAX;
        uint64_t height_ = UINT32_MAX;
        double widthMin_ = 10;
        double widthMax_ = 500;
        double scaleFactor_ = 0.;
        uint64_t makespanWidth_ = 0;

        /* === Private method(s) === */

        void headerPrinter(std::ofstream &file) const;

        void axisPrinter(std::ofstream &file) const;

        void jobPrinter(std::ofstream &file, const sched::Job &job) const;
    };

    /* === Inline method(s) === */
}
#endif //SPIDER2_SVGGANTTEXPORTER_H