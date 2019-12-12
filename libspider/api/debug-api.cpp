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

#include <common/Logger.h>
#include <common/Exception.h>
#include <api/debug-api.h>
#include <api/pisdf-api.h>
#include <graphs-tools/transformation/optims/PiSDFGraphOptimizer.h>
#include <graphs-tools/exporter/DOTExporter.h>

/* === Methods implementation === */

void spider::api::exportPostExecGantt(const std::string &) {
    if (!exportTrace()) {
        throwSpiderException("Can not export graph if traces are disable. Use Spider::enableTrace().");
    }

    /* == Open the file for the gantt == */

    /* == Get the execution traces == */

    /* == Print the gantt == */

    /* == Close the file == */
}

void spider::api::exportPreExecGantt(const std::string &) {
    /* == Open the file for the gantt == */

    /* == Get the schedule == */

    /* == Print the gantt == */

    /* == Close the file == */
}

void spider::api::exportSRDAG(const std::string &, pisdf::Graph *) {
    /* == Get the PiSDF graph and transform it to SR-DAG == */

//    if (Spider::API::srdagOptim()) {
//        pisdf::GraphOptimizer()(srdag);
//    }
//
//    /* == Print the SR-DAG == */
//    Spider::PiSDF::DOTExporter(srdag).print(path);
}

void spider::api::exportGraphToDOT(const std::string &path, pisdf::Graph *graph) {
    /* == Print the Graph == */
    pisdf::DOTExporter(graph).print(path);
}

void spider::api::enableLogger(log::Type type) {
    switch (type) {
        case log::Type::LRT:
            log::enable<log::Type::LRT>();
            break;
        case log::Type::TIME:
            log::enable<log::Type::TIME>();
            break;
        case log::Type::GENERAL:
            log::enable<log::Type::GENERAL>();
            break;
        case log::Type::SCHEDULE:
            log::enable<log::Type::SCHEDULE>();
            break;
        case log::Type::MEMORY:
            log::enable<log::Type::MEMORY>();
            break;
        case log::Type::TRANSFO:
            log::enable<log::Type::TRANSFO>();
            break;
        case log::Type::OPTIMS:
            log::enable<log::Type::OPTIMS>();
            break;
        case log::Type::EXPR:
            log::enable<log::Type::EXPR>();
            break;
    }
}

void spider::api::disableLogger(log::Type type) {
    switch (type) {
        case log::Type::LRT:
            log::disable<log::Type::LRT>();
            break;
        case log::Type::TIME:
            log::disable<log::Type::TIME>();
            break;
        case log::Type::GENERAL:
            log::disable<log::Type::GENERAL>();
            break;
        case log::Type::SCHEDULE:
            log::disable<log::Type::SCHEDULE>();
            break;
        case log::Type::MEMORY:
            log::disable<log::Type::MEMORY>();
            break;
        case log::Type::TRANSFO:
            log::disable<log::Type::TRANSFO>();
            break;
        case log::Type::OPTIMS:
            log::disable<log::Type::OPTIMS>();
            break;
        case log::Type::EXPR:
            log::disable<log::Type::EXPR>();
            break;
    }
}