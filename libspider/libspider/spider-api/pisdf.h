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
#ifndef SPIDER2_PISDF_H
#define SPIDER2_PISDF_H

/* === Include(s) === */

#include <cstdint>
#include <string>
#include <spider-api/general.h>
#include <graphs/pisdf/Types.h>

/* === API methods === */

namespace Spider {

    /**
     * @brief Get the user defined graph of the Spider session.
     * @return
     */
    PiSDF::Graph *&pisdfGraph();

    namespace API {

        /* === Graph API === */

        PiSDFGraph *createGraph(std::string name,
                                std::uint32_t actorCount = 0,
                                std::uint32_t edgeCount = 0,
                                std::uint32_t paramCount = 0,
                                std::uint32_t inIFCount = 0,
                                std::uint32_t outIFCount = 0,
                                std::uint32_t cfgActorCount = 0,
                                StackID stack = StackID::PISDF);

        PiSDFGraph *createSubraph(PiSDFGraph *graph,
                                  std::string name,
                                  std::uint32_t actorCount = 0,
                                  std::uint32_t edgeCount = 0,
                                  std::uint32_t paramCount = 0,
                                  std::uint32_t inIFCount = 0,
                                  std::uint32_t outIFCount = 0,
                                  std::uint32_t cfgActorCount = 0,
                                  StackID stack = StackID::PISDF);

        // TODO: add function call.
        PiSDFVertex *createVertex(PiSDFGraph *graph,
                                  std::string name,
                                  std::uint32_t edgeINCount = 0,
                                  std::uint32_t edgeOUTCount = 0,
                                  StackID stack = StackID::PISDF);

        PiSDFJoinVertex *createJoin(PiSDFGraph *graph,
                                    std::string name,
                                    std::uint32_t edgeINCount = 0,
                                    StackID stack = StackID::PISDF);

        PiSDFForkVertex *createFork(PiSDFGraph *graph,
                                    std::string name,
                                    std::uint32_t edgeOUTCount = 0,
                                    std::uint32_t nParamsIN = 0,
                                    StackID stack = StackID::PISDF);

        PiSDFTailVertex *createTail(PiSDFGraph *graph,
                                    std::string name,
                                    std::uint32_t edgeINCount = 0,
                                    std::uint32_t nParamsIN = 0,
                                    StackID stack = StackID::PISDF);

        PiSDFHeadVertex *createHead(PiSDFGraph *graph,
                                    std::string name,
                                    std::uint32_t edgeINCount = 0,
                                    std::uint32_t nParamsIN = 0,
                                    StackID stack = StackID::PISDF);

        PiSDFDuplicateVertex *createDuplicate(PiSDFGraph *graph,
                                              std::string name,
                                              std::uint32_t edgeOUTCount = 0,
                                              std::uint32_t nParamsIN = 0,
                                              StackID stack = StackID::PISDF);

        PiSDFUpSampleVertex *createUpsample(PiSDFGraph *graph,
                                            std::string name,
                                            std::uint32_t nParamsIN = 0,
                                            StackID stack = StackID::PISDF);

        PiSDFDownSampleVertex *createDownsample(PiSDFGraph *graph,
                                                std::string name,
                                                std::uint32_t nParamsIN = 0,
                                                StackID stack = StackID::PISDF);

        PiSDFInitVertex *createInit(PiSDFGraph *graph,
                                    std::string name,
                                    std::uint32_t nParamsIN = 0,
                                    StackID stack = StackID::PISDF);

        PiSDFEndVertex *createEnd(PiSDFGraph *graph,
                                  std::string name,
                                  std::uint32_t nParamsIN = 0,
                                  StackID stack = StackID::PISDF);

        // TODO: add function call
        PiSDFVertex *createConfigActor(PiSDFGraph *graph,
                                       std::string name,
                                       std::uint32_t edgeINCount = 0,
                                       std::uint32_t edgeOUTCount = 0,
                                       std::uint32_t nParamsIN = 0,
                                       std::uint32_t nParamsOUT = 0,
                                       StackID stack = StackID::PISDF);

        PiSDFInputInterface *createInputInterface(PiSDFGraph *graph,
                                                  std::string name,
                                                  StackID stack = StackID::PISDF);

        PiSDFOutputInterface *createOutputInterface(PiSDFGraph *graph,
                                                    std::string name,
                                                    StackID stack = StackID::PISDF);

        /* === Param API === */

        PiSDFParam *createStaticParam(PiSDFGraph *graph,
                                      std::string name,
                                      std::int64_t value,
                                      StackID stack = StackID::PISDF);

        PiSDFParam *createStaticParam(PiSDFGraph *graph,
                                      std::string name,
                                      std::string expression,
                                      StackID stack = StackID::PISDF);

        PiSDFDynamicParam *createDynamicParam(PiSDFGraph *graph,
                                              std::string name,
                                              StackID stack = StackID::PISDF);

        PiSDFDynamicParam *createDynamicParam(PiSDFGraph *graph,
                                              std::string name,
                                              std::string expression,
                                              StackID stack = StackID::PISDF);

        PiSDFInHeritedParam *createInheritedParam(PiSDFGraph *graph,
                                                  std::string name,
                                                  PiSDFParam *parent,
                                                  StackID stack = StackID::PISDF);

        /* === Edge API === */

        PiSDFEdge *createEdge(PiSDFAbstractVertex *source,
                              std::uint16_t srcPortIx,
                              std::string srcRateExpression,
                              PiSDFAbstractVertex *sink,
                              std::uint16_t snkPortIx,
                              std::string snkRateExpression,
                              StackID stack = StackID::PISDF);

        PiSDFEdge *createEdge(PiSDFAbstractVertex *source,
                              std::uint16_t srcPortIx,
                              std::int64_t srcRate,
                              PiSDFAbstractVertex *sink,
                              std::uint16_t snkPortIx,
                              std::int64_t snkRate,
                              StackID stack = StackID::PISDF);

        PiSDFDelay *createDelay(PiSDFEdge *edge,
                                std::string delayExpression,
                                PiSDFVertex *setter = nullptr,
                                std::uint32_t setterPortIx = 0,
                                const std::string &setterRateExpression = "0",
                                PiSDFVertex *getter = nullptr,
                                std::uint32_t getterPortIx = 0,
                                const std::string &getterRateExpression = "0",
                                bool persistent = true,
                                StackID stack = StackID::PISDF);

        PiSDFDelay *createDelay(PiSDFEdge *edge,
                                std::int64_t delayValue,
                                PiSDFVertex *setter = nullptr,
                                std::uint32_t setterPortIx = 0,
                                std::int64_t setterRate = 0,
                                PiSDFVertex *getter = nullptr,
                                std::uint32_t getterPortIx = 0,
                                std::int64_t getterRate = 0,
                                bool persistent = true,
                                StackID stack = StackID::PISDF);
    }
}

#endif //SPIDER2_PISDF_H
