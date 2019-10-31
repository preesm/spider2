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

#include <cinttypes>
#include <graphs-tools/transformation/SRDAGTransformation.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs-tools/brv/LCMBRVCompute.h>

/* === Methods implementation === */

std::pair<Spider::SRDAG::JobStack, Spider::SRDAG::JobStack>
Spider::SRDAG::staticSingleRateTransformation(const Spider::SRDAG::Job &job, PiSDFGraph *srdag) {
    if (!srdag) {
        throwSpiderException("nullptr for single rate graph.");
    }
    if (!job.reference_) {
        throwSpiderException("nullptr for job.reference graph.");
    }

    auto *srdagInstance = srdag;
    if (job.srdagIx_ >= 0) {
        // TODO: update parameters values of job.reference_ using the one of current instance
        srdagInstance = srdag->subgraphs()[job.srdagIx_];
        if (!srdagInstance || (srdagInstance->reference() != job.reference_)) {
            throwSpiderException("could not find matching single rate instance [%"
                                         PRIu32
                                         "] of graph [%s]", job.instanceValue_, job.reference_->name().c_str());
        }
    }

    /* == Compute the repetition values of the graph (if dynamic and/or first instance) == */
    if (job.reference_->dynamic() || job.srdagIx_ <= 0) {
        LCMBRVCompute brvTask{job.reference_};
        brvTask.execute();
    }

    /* == Do the linkage for every edges of the graph == */
    JobStack nextJobs;
    JobStack dynaJobs;

    return std::make_pair(std::move(nextJobs), std::move(dynaJobs));
}
