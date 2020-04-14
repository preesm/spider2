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
/* === Includes === */

#include <api/config-api.h>
#include <common/Logger.h>
#include <common/EnumIterator.h>

/* === Static variable(s) definition === */

struct SpiderConfiguration {
    bool optimizeSRDAG_ = true;
    bool verbose_ = false;
    bool exportTrace_ = false;
    bool exportSRDAG_ = false;
    bool exportGantt_ = false;
    bool useSVGGanttExporter_ = false;
};

static SpiderConfiguration config_;

/* === Methods implementation === */

void spider::api::enableExportTrace() {
    config_.exportTrace_ = true;
}

void spider::api::disableExportTrace() {
    config_.exportTrace_ = false;
}

void spider::api::enableExportSRDAG() {
    config_.exportSRDAG_ = true;
}

void spider::api::disableExportSRDAG() {
    config_.exportSRDAG_ = false;
}

void spider::api::enableVerbose() {
    config_.verbose_ = true;
    for (auto log: EnumIterator<log::Type>()) {
        enableLogger(log);
    }
}

void spider::api::disableVerbose() {
    config_.verbose_ = false;
    for (auto log: EnumIterator<log::Type>()) {
        disableLogger(log);
    }
}

void spider::api::enableSRDAGOptims() {
    config_.optimizeSRDAG_ = true;
}

void spider::api::disableSRDAGOptims() {
    config_.optimizeSRDAG_ = false;
}

void spider::api::enableExportGantt() {
    config_.exportGantt_ = true;
}

void spider::api::disableExportGantt() {
    config_.exportGantt_ = false;
}

void spider::api::useSVGGanttExporter() {
    config_.useSVGGanttExporter_ = true;
}

bool spider::api::exportTraceEnabled() {
    return config_.exportTrace_;
}

bool spider::api::exportSRDAGEnabled() {
    return config_.exportSRDAG_;
}

bool spider::api::verboseEnabled() {
    return config_.verbose_;
}

bool spider::api::shouldOptimizeSRDAG() {
    return config_.optimizeSRDAG_;
}

bool spider::api::exportGanttEnabled() {
    return config_.exportGantt_;
}

bool spider::api::useSVGOverXMLGantt() {
    return config_.useSVGGanttExporter_;
}
