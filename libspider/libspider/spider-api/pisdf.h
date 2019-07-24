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
#ifndef SPIDER2_PISDF_H
#define SPIDER2_PISDF_H

/* === Include(s) === */

#include <cstdint>
#include <string>

/* === Forward declaration(s) === */

class PiSDFVertex;

class PiSDFParam;

class PiSDFGraph;

class PiSDFEdge;

class PiSDFDelay;

class PiSDFInterface;

/* === Type declaration(s) == */

using ParamInt64 = std::int64_t;
using ParamInt32 = std::int32_t;

/* === API methods === */

namespace Spider {
    namespace API {

        /* === Graph API === */

        PiSDFGraph *createGraph(const std::string &name,
                                std::uint64_t nActors,
                                std::uint64_t nEdges,
                                std::uint64_t nParams = 0,
                                std::uint64_t nInputInterfaces = 0,
                                std::uint64_t nOutputInterfaces = 0,
                                std::uint64_t nConfigActors = 0);

        PiSDFGraph *createSubraph(PiSDFGraph *graph,
                                  const std::string &name,
                                  std::uint64_t nActors,
                                  std::uint64_t nEdges,
                                  std::uint64_t nParams = 0,
                                  std::uint64_t nInputInterfaces = 0,
                                  std::uint64_t nOutputInterfaces = 0,
                                  std::uint64_t nConfigActors = 0);

        // TODO: add function call.
        PiSDFVertex *createVertex(PiSDFGraph *graph,
                                  const std::string &name,
                                  std::uint32_t nEdgesIN = 0,
                                  std::uint32_t nEdgesOUT = 0,
                                  std::uint32_t nParamsIN = 0);

        PiSDFVertex *createBroadcast(PiSDFGraph *graph,
                                     const std::string &name,
                                     std::uint32_t nEdgesOUT = 0,
                                     std::uint32_t nParamsIN = 0);

        PiSDFVertex *createFork(PiSDFGraph *graph,
                                const std::string &name,
                                std::uint32_t nEdgesOUT = 0,
                                std::uint32_t nParamsIN = 0);

        PiSDFVertex *createRoundbuffer(PiSDFGraph *graph,
                                       const std::string &name,
                                       std::uint32_t nEdgesIN = 0,
                                       std::uint32_t nParamsIN = 0);

        PiSDFVertex *createJoin(PiSDFGraph *graph,
                                const std::string &name,
                                std::uint32_t nEdgesIN = 0,
                                std::uint32_t nParamsIN = 0);

        PiSDFVertex *createInit(PiSDFGraph *graph,
                                const std::string &name,
                                std::uint32_t nParamsIN = 0);

        PiSDFVertex *createEnd(PiSDFGraph *graph,
                               const std::string &name,
                               std::uint32_t nParamsIN = 0);

        // TODO: add function call
        PiSDFVertex *createConfigActor(PiSDFGraph *graph,
                                       const std::string &name,
                                       std::uint32_t nEdgesIN = 0,
                                       std::uint32_t nEdgesOUT = 0,
                                       std::uint32_t nParamsIN = 0,
                                       std::uint32_t nParamsOUT = 0);

        PiSDFInterface *createInputInterface(PiSDFGraph *graph,
                                             const std::string &name);

        PiSDFInterface *createOutputInterface(PiSDFGraph *graph,
                                              const std::string &name);

        /* === Param API === */

        PiSDFParam *createStaticParam(PiSDFGraph *graph,
                                      const std::string &name,
                                      ParamInt64 value);

        PiSDFParam *createDynamicParam(PiSDFGraph *graph,
                                       const std::string &name,
                                       PiSDFVertex *setter);

        PiSDFParam *createDependentParam(PiSDFGraph *graph,
                                         const std::string &name,
                                         const std::string &expression);

        PiSDFParam *createInheritedParam(PiSDFGraph *graph,
                                         const std::string &name,
                                         PiSDFParam *parent);

        /* === Edge API === */

        PiSDFEdge *createEdge(PiSDFGraph *graph,
                              PiSDFVertex *source,
                              std::uint16_t srcPortIx,
                              const std::string &srcRateExpression,
                              PiSDFVertex *sink,
                              std::uint16_t snkPortIx,
                              const std::string &snkRateExpression);

        PiSDFEdge *createEdge(PiSDFGraph *graph,
                              PiSDFVertex *source,
                              std::uint16_t srcPortIx,
                              std::int64_t srcRate,
                              PiSDFVertex *sink,
                              std::uint16_t snkPortIx,
                              std::int64_t snkRate);

        PiSDFEdge *createEdge(PiSDFGraph *graph,
                              PiSDFVertex *source,
                              std::uint16_t srcPortIx,
                              std::int64_t srcRate,
                              PiSDFVertex *sink,
                              std::uint16_t snkPortIx,
                              const std::string &snkRateExpression);

        PiSDFEdge *createEdge(PiSDFGraph *graph,
                              PiSDFVertex *source,
                              std::uint16_t srcPortIx,
                              const std::string &srcRateExpression,
                              PiSDFVertex *sink,
                              std::uint16_t snkPortIx,
                              std::int64_t snkRate);

        PiSDFDelay *createDelay(PiSDFEdge *edge,
                                const std::string &delayExpression,
                                bool persistent = true,
                                PiSDFVertex *setter = nullptr,
                                PiSDFVertex *getter = nullptr);

        PiSDFDelay *createDelay(PiSDFEdge *edge,
                                std::int64_t delayValue,
                                bool persistent = true,
                                PiSDFVertex *setter = nullptr,
                                PiSDFVertex *getter = nullptr);
    }
}

#endif //SPIDER2_PISDF_H
