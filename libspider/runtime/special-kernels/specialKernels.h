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
#ifndef SPIDER2_SPECIALKERNELS_H
#define SPIDER2_SPECIALKERNELS_H

/* === Include(s) === */

#include <cstdint>

/* === Function(s) prototype === */

namespace spider {
    namespace rt {
        /**
         * @brief Default special kernel for @refitem pisdf::VertexType::JOIN actors.
         * @details Copy input buffers in output buffers in a serial manner.
         *          paramsIn[0]     = rate of the output buffer.
         *          paramsIn[1]     = number of input buffers.
         *          paramsIn[i + 2] = size of in[i] to copy.
         * @param paramsIn Input parameters.
         * @param in       Input buffers.
         * @param out      Output buffers.
         */
        void join(const int64_t *paramsIn, int64_t *, void *in[], void *out[]);

        /**
         * @brief Default special kernel for @refitem pisdf::VertexType::FORK actors.
         * @details Copy input buffers in output buffers in a serial manner.
         *          paramsIn[0]     = rate of the input buffer.
         *          paramsIn[1]     = number of output buffers.
         *          paramsIn[i + 2] = size of out[i] to copy.
         * @param paramsIn Input parameters.
         * @param in       Input buffers.
         * @param out      Output buffers.
         */
        void fork(const int64_t *paramsIn, int64_t *, void *in[], void *out[]);

        /**
         * @brief Default special kernel for @refitem pisdf::VertexType::HEAD actors.
         * @details Copy first bytes of inputs into output buffers.
         *          paramsIn[0]     = number of input buffers to be considered.
         *          paramsIn[i + 1] = size of in[i] to copy.
         * @param paramsIn Input parameters.
         * @param in       Input buffers.
         * @param out      Output buffers.
         */
        void head(const int64_t *paramsIn, int64_t *, void *in[], void *out[]);

        /**
         * @brief Default special kernel for @refitem pisdf::VertexType::TAIL actors.
         * @details Copy last bytes of inputs into output buffers.
         *          paramsIn[0]     = total number of input buffers.
         *          paramsIn[1]     = index of the first input buffer to consider.
         *          paramsIn[2]     = offset in the first input buffer to apply.
         *          paramsIn[3]     = effective size to copy from first input buffer.
         *          paramsIn[i + 4] = size of in[paramsIn[1] + i] to copy.
         * @param paramsIn Input parameters.
         * @param in       Input buffers.
         * @param out      Output buffers.
         */
        void tail(const int64_t *paramsIn, int64_t *, void *in[], void *out[]);

        /**
         * @brief Default special kernel for @refitem pisdf::VertexType::DUPLICATE actors.
         * @details Copy content of input buffer onto every output buffers.
         *          paramsIn[0] = total number of output buffers.
         *          paramsIn[1] = size of the input buffer.
         * @param paramsIn Input parameters.
         * @param in       Input buffers.
         * @param out      Output buffers.
         */
        void duplicate(const int64_t *paramsIn, int64_t *, void *in[], void *out[]);

        /**
         * @brief Default special kernel for @refitem pisdf::VertexType::REPEAT actors.
         * @details Repeat content of input buffer into output buffer with respect to output size.
         *          paramsIn[0] = size of the input buffer.
         *          paramsIn[1] = size of the output buffer.
         *    if input size >= output size, then only first output size bytes are copied.
         *    else input is copied in a circular manner.
         *    ex: input size = 5
         *        input = { 3; 1; 4; 1; 5}
         *        output size = 8
         *        output = { 3; 1; 4; 1; 5; 3; 1; 4}
         * @param paramsIn Input parameters.
         * @param in       Input buffers.
         * @param out      Output buffers.
         */
        void repeat(const int64_t *paramsIn, int64_t *, void *in[], void *out[]);

        /**
         * @brief Default special kernel for @refitem pisdf::VertexType::INIT actors.
         * @details Set output to 0 if linked-delay is not persistent.
         *          Copy content of delay buffer into output buffer if linked-delay is persistent.
         *          paramsIn[0] = persistence property of the delay.
         *          paramsIn[1] = size of the delay.
         *          paramsIn[2] = address of the delay buffer (if persistent).
         * @param paramsIn Input parameters.
         * @param out      Output buffers.
         */
        void init(const int64_t *paramsIn, int64_t *, void *[], void *out[]);

        /**
         * @brief Default special kernel for @refitem pisdf::VertexType::END actors.
         * @details Do nothing if linked-delay is not persistent.
         *          Copy content of input buffer to the delay buffer if linked-delay is persistent.
         *          paramsIn[0] = persistence property of the delay.
         *          paramsIn[1] = size of the delay.
         *          paramsIn[2] = address of the delay buffer (if persistent).
         * @param paramsIn Input parameters.
         * @param in       Input buffers.
         */
        void end(const int64_t *paramsIn, int64_t *, void *in[], void *[]);
    }
}

#endif //SPIDER2_SPECIALKERNELS_H
