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

#include <common/Logger.h>
#include <common/Exception.h>
#include <common/Exporter.h>
#include <api/debug-api.h>
#include <api/pisdf-api.h>
#include <api/config-api.h>

#ifndef _NO_BUILD_GRAPH_EXPORTER

#include <graphs-tools/exporter/PiSDFDOTExporter.h>

#endif

/* === Methods implementation === */

#ifndef _NO_BUILD_GRAPH_EXPORTER
void spider::api::exportGraphToDOT(pisdf::Graph *graph, const std::string &path) {
    /* == Print the Graph == */
    auto exporter = pisdf::PiSDFDOTExporter(graph);
    exporter.printFromPath(path);
#else

void spider::api::exportGraphToDOT(pisdf::Graph *, const std::string &) {
    printer::fprintf(stderr, "Graph exporter is not built. Recompile spider2 with -DBUILD_GRAPH_EXPORTER=ON option.\n");
#endif
}

void spider::api::enableLogger(log::Type type) {
    switch (type) {
        case log::Type::LRT:
            log::enable<log::LRT>();
            break;
        case log::Type::TIME:
            log::enable<log::TIME>();
            break;
        case log::Type::GENERAL:
            log::enable<log::GENERAL>();
            break;
        case log::Type::SCHEDULE:
            log::enable<log::SCHEDULE>();
            break;
        case log::Type::MEMORY:
            log::enable<log::MEMORY>();
            break;
        case log::Type::TRANSFO:
            log::enable<log::TRANSFO>();
            break;
        case log::Type::OPTIMS:
            log::enable<log::OPTIMS>();
            break;
        case log::Type::EXPR:
            log::enable<log::EXPR>();
            break;
        default:
            printer::fprintf(stderr, "Unsupported logger enum type.\n");
    }
}

void spider::api::disableLogger(log::Type type) {
    switch (type) {
        case log::Type::LRT:
            log::disable<log::LRT>();
            break;
        case log::Type::TIME:
            log::disable<log::TIME>();
            break;
        case log::Type::GENERAL:
            log::disable<log::GENERAL>();
            break;
        case log::Type::SCHEDULE:
            log::disable<log::SCHEDULE>();
            break;
        case log::Type::MEMORY:
            log::disable<log::MEMORY>();
            break;
        case log::Type::TRANSFO:
            log::disable<log::TRANSFO>();
            break;
        case log::Type::OPTIMS:
            log::disable<log::OPTIMS>();
            break;
        case log::Type::EXPR:
            log::disable<log::EXPR>();
            break;
        default:
            printer::fprintf(stderr, "Unsupported logger enum type.\n");
    }
}

void spider::api::setLoggerSteam(FILE *stream) {
    log::setOutputStream(stream);
}
