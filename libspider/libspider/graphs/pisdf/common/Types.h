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
#ifndef SPIDER2_TYPES_H
#define SPIDER2_TYPES_H

/* === Includes === */

#include <cstdint>

/* === Methods prototype === */

namespace Spider {
    namespace PiSDF {

        /* === Enumeration(s) === */

        /**
         * @brief PiSDF parameter types
         */
        enum class ParamType : std::uint8_t {
            STATIC,            /*! Static parameter: expression is evaluated at startup only once */
            DYNAMIC,           /*! Dynamic parameter: value is set at runtime */
            INHERITED,         /*! Inherited parameter: value depend on parent */
        };

        /**
         * @brief Type of PiSDF vertices
         */
        enum class VertexType : std::uint8_t {
            NORMAL,         /*! Normal actor type */
            SPECIAL,        /*! Special actor type */
            CONFIG,         /*! Config vertex type */
            GRAPH,          /*! Graph vertex type */
            INTERFACE,      /*! Interface vertex type */
            DELAY,          /*! Delay vertex type */
            FORK,           /*! Fork actor subtype */
            JOIN,           /*! Join actor subtype */
            UPSAMPLE,       /*! Up-sample actor subtype */
            DUPLICATE,      /*! Duplicate actor subtype */
            TAIL,           /*! Tail actor subtype */
            HEAD,           /*! Head actor subtype */
            INIT,           /*! Init actor subtype */
            END,            /*! End actor subtype */
            INPUT,          /*! Input interface type */
            OUTPUT,         /*! Output interface type */
        };

        /* === Forward declaration(s) === */

        class Vertex;

        class ExecVertex;

        class JoinVertex;

        class ForkVertex;

        class TailVertex;

        class HeadVertex;

        class DuplicateVertex;

        class UpSampleVertex;

        class InitVertex;

        class EndVertex;

        class Graph;

        class Param;

        class DynamicParam;

        class InHeritedParam;

        class Edge;

        class Delay;

        class Interface;

        class InputInterface;

        class OutputInterface;
    }
}

/* === Simpler accessor to namespace === */

/* == Vertex definitions == */
using PiSDFAbstractVertex = Spider::PiSDF::Vertex;
using PiSDFVertex = Spider::PiSDF::ExecVertex;
using PiSDFJoinVertex = Spider::PiSDF::JoinVertex;
using PiSDFForkVertex = Spider::PiSDF::ForkVertex;
using PiSDFTailVertex = Spider::PiSDF::TailVertex;
using PiSDFHeadVertex = Spider::PiSDF::HeadVertex;
using PiSDFDuplicateVertex = Spider::PiSDF::DuplicateVertex;
using PiSDFUpSampleVertex = Spider::PiSDF::UpSampleVertex;
using PiSDFInitVertex = Spider::PiSDF::InitVertex;
using PiSDFEndVertex = Spider::PiSDF::EndVertex;

/* == Graph definition == */
using PiSDFGraph = Spider::PiSDF::Graph;

/* == Param definitions == */
using PiSDFParam = Spider::PiSDF::Param;
using PiSDFDynamicParam = Spider::PiSDF::DynamicParam;
using PiSDFInHeritedParam = Spider::PiSDF::InHeritedParam;

/* == Edge and Delay definitions == */
using PiSDFEdge = Spider::PiSDF::Edge;
using PiSDFDelay = Spider::PiSDF::Delay;

/* == Interface definitions == */
using PiSDFInterface = Spider::PiSDF::Interface;
using PiSDFInputInterface = Spider::PiSDF::InputInterface;
using PiSDFOutputInterface = Spider::PiSDF::OutputInterface;

/* == VertexType definition == */
using PiSDFVertexType = Spider::PiSDF::VertexType;

#endif //SPIDER2_TYPES_H
