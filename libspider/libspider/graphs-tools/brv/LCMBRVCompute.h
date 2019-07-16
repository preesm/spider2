/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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
#ifndef SPIDER2_LCMBRVCOMPUTE_H
#define SPIDER2_LCMBRVCOMPUTE_H

/* === Include(s) === */

#include <graphs-tools/brv/BRVCompute.h>
#include <common/Rational.h>

/* === Class definition === */

class LCMBRVCompute : public BRVCompute {
public:

    explicit LCMBRVCompute(PiSDFGraph *graph) : BRVCompute(graph) { };

    ~LCMBRVCompute() = default;

    /* === Method(s) === */

    void execute() override;

    /* === Getter(s) === */

    /* === Setter(s) === */

private:
    std::int64_t lcmFactor_ = 0;

    /* === Private method(s) === */

    /**
     * Fill an array of Rational relations between the different vertices of a given connected components based on their
     * production / consumption data rates.
     *
     * @param edgeArray Edge array of the connected component;
     * @param reps      Array of Rational to be filled.
     */
    void extractRationals(Spider::Array<const PiSDFEdge *> &edgeArray, Spider::Array<Spider::Rational> &reps) const;

    /**
     * @brief Compute the LCM factor of a connected component based on all Rationals.
     * @param component Current connected component.
     * @param reps      Array of Rationals.
     */
    void computeLCM(const BRVComponent &component, Spider::Array<Spider::Rational> &reps);

    /**
     * @brief Compute the repetition vector of a connected component based on all Rationals.
     * @param component Current connected component.
     * @param reps      Array of Rationals.
     */
    void computeBRV(const BRVComponent &component, Spider::Array<Spider::Rational> &reps) const;

    /**
     * Check the consistency of the connected components graph.
     * @param edgeArray  Edge array of the connected component;
     */
    void checkValidity(Spider::Array<const PiSDFEdge *> &edgeArray) const;
};

/* === Inline method(s) === */



#endif //SPIDER2_LCMBRVCOMPUTE_H
