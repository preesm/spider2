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
#ifndef SPIDER2_REFINEMENT_H
#define SPIDER2_REFINEMENT_H

/* === Include(s) === */

#include <containers/StlContainers.h>
#include <spider-api/pisdf.h>

namespace Spider {
    namespace PiSDF {

        /* === Forward declaration(s) === */

        class Param;

        inline void dummy(std::int64_t *[], std::int64_t *[], void *[], void *[]) { }

        /* === Class definition === */

        class Refinement {
        public:

            Refinement() = default;

            Refinement(const Refinement &) = default;

            Refinement(Refinement &&) = default;

            Refinement(std::uint32_t paramINCount,
                       std::uint32_t paramOUTCount,
                       callback fct,
                       std::string name = "unnamed-fct") : name_{std::move(name)}, fct_{fct} {
                inputParamsValue_.reserve(paramINCount);
                outputParamsValue_.reserve(paramOUTCount);
            }

            ~Refinement() = default;

            /* === Method(s) === */

            inline void operator()(Spider::vector<void *> &fifosIN, Spider::vector<void *> &fifosOUT) {
                fct_(inputParamsValue_.data(), outputParamsValue_.data(), fifosIN.data(), fifosOUT.data());
            }

            /* === Getter(s) === */

            inline const Spider::vector<std::int64_t *> &inputParamsValue() const;

            inline const Spider::vector<std::int64_t *> &outputParamsValue() const;

            /* === Setter(s) === */

            /**
             * @brief Add a parameter at the end of the input param vector.
             * @param param  Param to add.
             */
            inline void addInputParam(std::int64_t *param);

            /**
             * @brief Add a parameter at the end of the output param vector.
             * @param param  Param to add.
             */
            inline void addOutputParam(std::int64_t *param);

        private:
            Spider::vector<std::int64_t *> inputParamsValue_;
            Spider::vector<std::int64_t *> outputParamsValue_;
            std::string name_ = "unnamed-refinement";

            callback fct_ = dummy;

            /* === Private method(s) === */
        };

        /* === Inline method(s) === */

        const Spider::vector<std::int64_t *> &Refinement::inputParamsValue() const {
            return inputParamsValue_;
        }

        const Spider::vector<std::int64_t *> &Refinement::outputParamsValue() const {
            return outputParamsValue_;
        }

        void Refinement::addInputParam(std::int64_t *param) {
            if (inputParamsValue_.size() == inputParamsValue_.capacity()) {
                throwSpiderException("refinement [%s]: too many input params.", name_.c_str());
            }
            inputParamsValue_.emplace_back(param);
        }

        void Refinement::addOutputParam(std::int64_t *param) {
            if (outputParamsValue_.size() == outputParamsValue_.capacity()) {
                throwSpiderException("refinement [%s]: too many output params.", name_.c_str());
            }
            outputParamsValue_.emplace_back(param);
        }

    }
}

#endif //SPIDER2_REFINEMENT_H
