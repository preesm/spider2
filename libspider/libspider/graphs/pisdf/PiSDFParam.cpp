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

#include <graphs/pisdf/PiSDFGraph.h>
#include <graphs/pisdf/PiSDFParam.h>

/* === Methods implementation === */

PiSDFParam::PiSDFParam(PiSDFGraph *graph,
                       std::string name,
                       const std::string &expression) : graph_{graph},
                                                        name_{std::move(name)},
                                                        expression_{graph, expression} {
    if (expression_.isStatic()) {
        type_ = PiSDFParamType::STATIC;
    } else {
        type_ = PiSDFParamType::DYNAMIC_DEPENDENT;
    }

    /* == Transform the name to lower case for the expression parser == */
    std::transform(name_.begin(), name_.end(), name_.begin(), ::tolower);

    /* == Add the parameter to the graph == */
    graph->addParam(this);
}

PiSDFParam::PiSDFParam(PiSDFGraph *graph,
                       std::string name,
                       std::int64_t value) : graph_{graph},
                                             name_{std::move(name)},
                                             type_{PiSDFParamType::STATIC},
                                             expression_{value} {
    /* == Transform the name to lower case for the expression parser == */
    std::transform(name_.begin(), name_.end(), name_.begin(), ::tolower);

    /* == Add the parameter to the graph == */
    graph->addParam(this);
}

PiSDFParam::PiSDFParam(PiSDFGraph *graph, std::string name, const PiSDFParam *parent) : graph_{graph},
                                                                                        name_{std::move(name)},
                                                                                        type_{PiSDFParamType::HERITED},
                                                                                        expression_{0} {
    /* == Set the parent parameter == */
    if (!parent) {
        throwSpiderException("Parent parameter is nullptr");
    }
    inheritedParam_ = parent;

    /* == Transform the name to lower case for the expression parser == */
    std::transform(name_.begin(), name_.end(), name_.begin(), ::tolower);

    /* == Add the parameter to the graph == */
    graph->addParam(this);
}

PiSDFParam::PiSDFParam(PiSDFGraph *graph, std::string name, PiSDFVertex *setter) : graph_{graph},
                                                                                   name_{std::move(name)},
                                                                                   type_{PiSDFParamType::DYNAMIC},
                                                                                   setter_{setter},
                                                                                   expression_{0} {
    /* == Transform the name to lower case for the expression parser == */
    std::transform(name_.begin(), name_.end(), name_.begin(), ::tolower);

    /* == Add the parameter to the graph == */
    graph->addParam(this);
}