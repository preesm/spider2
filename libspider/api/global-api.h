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
#ifndef SPIDER2_GLOBAL_API_H
#define SPIDER2_GLOBAL_API_H

/* === Include(s) === */

/* === non-namespace forward declaration(s) === */

enum class FreeListPolicy;

/* === non-namespace Enumeration(s) === */

/**
 * @brief Stack ids
 */
enum class StackID : uint64_t {
    PISDF = 0,       /*!< Stack used for PISDF graph (should be static) */
    ARCHI,           /*!< Stack used for architecture (should be static) */
    TRANSFO,         /*!< Stack used for graph transformations */
    EXPRESSION,      /*!< Stack used for handling expression */
    SCHEDULE,        /*!< Stack used for scheduling */
    RUNTIME,         /*!< Stack used by LRTs */
    GENERAL,         /*!< General stack used for classic new / delete */
    CONSTRAINTS,     /*!< Stack used for the scenario (application constraints) */
    First = PISDF,   /*!< Sentry for EnumIterator::begin */
    Last = CONSTRAINTS, /*!< Sentry for EnumIterator::end */
};

/* === non-namespace constant(s) === */

constexpr size_t STACK_COUNT = static_cast<size_t>(StackID::Last) + 1;

namespace spider {

    /* === Forward declaration(s) === */

    class Platform;

    class Cluster;

    class PE;

    class MemoryUnit;

    namespace pisdf {

        /**
        * @brief PiSDF parameter types
        */
        enum class ParamType : uint8_t {
            STATIC,            /*! Static parameter: expression is evaluated at startup only once */
            DYNAMIC,           /*! Dynamic parameter: value is set at runtime */
            INHERITED,         /*! Inherited parameter: value depend on parent */
        };

        /**
         * @brief Type of PiSDF vertices
         */
        enum class VertexType : uint8_t {
            NORMAL,         /*! Normal actor type */
            CONFIG,         /*! Config vertex type */
            DELAY,          /*! Delay vertex type */
            FORK,           /*! Fork actor subtype */
            JOIN,           /*! Join actor subtype */
            REPEAT,         /*! Repeat actor subtype */
            DUPLICATE,      /*! Duplicate actor subtype */
            TAIL,           /*! Tail actor subtype */
            HEAD,           /*! Head actor subtype */
            INIT,           /*! Init actor subtype */
            END,            /*! End actor subtype */
            GRAPH,          /*! Graph vertex type */
            INPUT,          /*! Input interface type */
            OUTPUT,         /*! Output interface type */
            First = NORMAL,
            Last = OUTPUT,
        };

        template<VertexType> class SpecialVertex;

        using ConfigVertex = SpecialVertex<VertexType::CONFIG>;

        using DelayVertex = SpecialVertex<VertexType::DELAY>;

        using ForkVertex = SpecialVertex<VertexType::FORK>;

        using JoinVertex = SpecialVertex<VertexType::JOIN>;

        using HeadVertex = SpecialVertex<VertexType::HEAD>;

        using TailVertex = SpecialVertex<VertexType::TAIL>;

        using RepeatVertex = SpecialVertex<VertexType::REPEAT>;

        using DuplicateVertex = SpecialVertex<VertexType::DUPLICATE>;

        using InitVertex = SpecialVertex<VertexType::INIT>;

        using EndVertex = SpecialVertex<VertexType::END>;

        class Vertex;

        class ExecVertex;

        class Graph;

        class Param;

        class DynamicParam;

        class InHeritedParam;

        class Edge;

        class Delay;

        class Interface;

        class InputInterface;

        class OutputInterface;

        class Refinement;

    }

    /* === Enumeration(s) === */

    /**
     * @brief Spider Processing Element types.
     */
    enum class PEType {
        LRT,         /*!< PE is used as an LRT and does not perform any computation */
        PE,          /*!< PE is used for computation only and does not perform any job management */
        First = LRT, /*!< Sentry for EnumIterator::begin */
        Last = PE,   /*!< Sentry for EnumIterator::end */
    };

    namespace log {
        enum Type : int32_t {
            LRT = 0,        /*! LRT logger. When enabled, this will print LRT logged information. */
            TIME,           /*! TIME logger. When enabled this will print time logged information */
            GENERAL,        /*! GENERAL purpose logger, used for information about almost everything */
            SCHEDULE,       /*! SCHEDULE logger. When enabled, this will print Schedule logged information. */
            MEMORY,         /*! MEMORY logger. When enabled, this will print Memory logged information. */
            TRANSFO,        /*! TRANSFO logger. When enabled, this will print transformation logged information. */
            OPTIMS,         /*! OPTIMS logger. When enabled, this will print transformation logged information. */
            EXPR,           /*! EXPRESSION logger. When enabled, this will print expression-parser logged information. */
            First = LRT,    /*!< Sentry for EnumIterator::begin */
            Last = EXPR,    /*!< Sentry for EnumIterator::end */
        };
    }

    /**
     * @brief Allocator types
     */
    enum class AllocatorType {
        FREELIST,             /*!< (Dynamic) FreeList type allocator */
        GENERIC,              /*!< (Dynamic) Generic type allocator (=malloc) */
        LINEAR_STATIC,        /*!< (Static) Linear type allocator */
        First = FREELIST,     /*!< Sentry for EnumIterator::begin */
        Last = LINEAR_STATIC, /*!< Sentry for EnumIterator::end */
    };

    /* === Structure(s) === */

    struct PlatformConfig {
        uint32_t PECount;
        uint32_t PETypeCount;
        uint32_t memoryUnitCount;
    };

    namespace log {
        struct Log {
            const char *litteral_;
            bool enabled_;
        };
    }

    /* === Constant(s) === */

    namespace log {
        constexpr auto LOGGER_COUNT = static_cast<uint8_t >(Type::EXPR) + 1;
    }

    constexpr size_t ALLOCATOR_COUNT = static_cast<size_t>(AllocatorType::Last) + 1;

    namespace pisdf {
        constexpr auto SPECIAL_VERTEX_COUNT =
                static_cast<uint8_t>(VertexType::END) - static_cast<uint8_t>(VertexType::CONFIG) + 1;
        constexpr auto VERTEX_TYPE_COUNT =
                static_cast<uint8_t>(VertexType::Last) - static_cast<uint8_t>(VertexType::First) + 1;
    }

    /* === Type definition(s) === */

    /**
     * @brief overridable communication cost routine.
     */
    using CommunicationCostRoutine = uint64_t (*)(
            /* = Number of bytes  = */   uint64_t
                                                 );

    /**
     * @brief cluster to cluster overridable cost routine.
     */
    using CommunicationCostRoutineC2C = uint64_t (*)(
            /* = Source Cluster ix  = */   uint32_t,
            /* = Sink Cluster ix    = */   uint32_t,
            /* = Number of bytes    = */   uint64_t
                                                    );

    /**
     * @brief Generic refinement used by Spider for the actors.
     */
    using callback = void (*)(const int64_t *, int64_t *[], void *[], void *[]);
}

#endif //SPIDER2_GLOBAL_API_H
