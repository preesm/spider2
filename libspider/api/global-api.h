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

#include <cstdint>
#include <cstddef>
#include <array>

/* === non-namespace Enumeration(s) === */

/**
 * @brief Stack ids
 */
enum class StackID : uint_least64_t {
    PISDF = 0,       /*!< Stack used for PISDF graph (should be static) */
    ARCHI,           /*!< Stack used for architecture (should be static) */
    TRANSFO,         /*!< Stack used for graph transformations */
    OPTIMS,          /*!< Stack used for graph optimizations */
    EXPRESSION,      /*!< Stack used for handling expression */
    SCHEDULE,        /*!< Stack used for scheduling */
    RUNTIME,         /*!< Stack used by LRTs */
    GENERAL,         /*!< General stack used for classic new / delete */
    First = PISDF,   /*!< Sentry for EnumIterator::begin */
    Last = GENERAL,  /*!< Sentry for EnumIterator::end */
};

/* === non-namespace constant(s) === */

constexpr size_t STACK_COUNT = static_cast<size_t>(StackID::Last) + 1;

namespace spider {

    /* === Forward declaration(s) === */

    class Platform;

    class Cluster;

    class PE;

    class MemoryInterface;

    class MemoryBus;

    class InterMemoryBus;

    class Stack;

    class RTKernel;

    class RTRunner;

    class RTPlatform;

    class RTCommunicator;

    namespace pisdf {

        /**
        * @brief PiSDF parameter types
        */
        enum class ParamType : uint_least8_t {
            STATIC,            /*! Static parameter: expression is evaluated at startup only once */
            DYNAMIC,           /*! Dynamic parameter: value is set at runtime */
            INHERITED,         /*! Inherited parameter: value depend on parent */
        };

        /**
         * @brief Type of PiSDF vertices
         */
        enum class VertexType : uint_least8_t {
            NORMAL,         /*! Normal actor type */
            CONFIG,         /*! Config vertex type */
            DELAY,          /*! Delay vertex type */
            FORK,           /*! Fork actor subtype */
            JOIN,           /*! Join actor subtype */
            REPEAT,         /*! Repeat actor subtype */
            DUPLICATE,      /*! Duplicate actor subtype */
            TAIL,           /*! Tail actor subtype */
            HEAD,           /*! Head actor subtype */
            EXTERN_IN,      /*! Extern output interface subtype */
            EXTERN_OUT,     /*! Extern output interface subtype */
            INIT,           /*! Init actor subtype */
            END,            /*! End actor subtype */
            GRAPH,          /*! Graph vertex type */
            INPUT,          /*! Input interface type */
            OUTPUT,         /*! Output interface type */
            First = NORMAL,
            Last = OUTPUT,
        };

        class Vertex;

        class ExecVertex;

        class ExternInterface;

        class DelayVertex;

        class NonExecVertex;

        class Graph;

        class Param;

        class DynamicParam;

        class InHeritedParam;

        class Edge;

        class Delay;

        class Interface;
    }

    /* === Enumeration(s) === */

    /**
     * @brief Spider run modes (INFINITE or LOOP).
     */
    enum class RunMode {
        INFINITE = 0, /*!< Run the application graph in a infinite loop */
        LOOP,         /*!< Run the application graph in a fixed size loop */
        EXTERN_LOOP,  /*!< Run the application graph in a extern fixed size loop */
    };

    /**
     * @brief Spider runtime algorithms.
     */
    enum class RuntimeType {
        JITMS = 0,  /*!< Use the Just In Time Multicore Scheduling runtime
                     *   see: https://tel.archives-ouvertes.fr/tel-01301642/file/These_HEULOT_Julien.pdf
                     */
        FAST_JITMS, /*!< Use a faster version of the JITMS runtime by not computing the Single Rate intermediate graph.
                     *   see: https://hal-univ-rennes1.archives-ouvertes.fr/hal-02355636
                     */
    };

    /**
     * @brief Spider scheduling algorithms.
     */
    enum class SchedulingAlgorithm {
        LIST_BEST_FIT,        /*!< Use a list based algorithm with best fit mapping decision (use srdag) */
        LIST_ROUND_ROBIN,     /*!< Use a list based algorithm with round robin mapping decision (use srdag) */
        GREEDY,               /*!< Greedy algorithm with no heuristics (use srdag) */
        SRLESS_LIST_BEST_FIT, /*!< Use a list based algorithm with best fit mapping decision (srdag less version) */
    };

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
        enum Type : int_least32_t {
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
     * @brief Allocator policies
     */
    enum class AllocatorPolicy {
        FREELIST_FIND_FIRST,         /*!< (Dynamic) FreeList with FIND_FIRST policy allocator policy */
        FREELIST_FIND_BEST,          /*!< (Dynamic) FreeList with FIND_BEST policy allocator policy */
        GENERIC,                     /*!< (Dynamic) Generic allocator policy (=malloc) */
        LINEAR_STATIC,               /*!< (Static) Linear allocator policy */
        First = FREELIST_FIND_FIRST, /*!< Sentry for EnumIterator::begin */
        Last = LINEAR_STATIC,        /*!< Sentry for EnumIterator::end */
    };

    /* === Structure(s) === */

    struct PlatformConfig {
        uint_least32_t PECount;
        uint_least32_t PETypeCount;
        uint_least32_t memoryUnitCount;
    };

    namespace log {
        struct Log {
            const char *litteral_;
            bool enabled_;
        };
    }

    /* === Constant(s) === */

    namespace log {
        constexpr auto LOGGER_COUNT = static_cast<uint_least8_t>(Type::EXPR) + 1;
    }

    constexpr size_t ALLOCATOR_POLICY_COUNT = static_cast<size_t>(AllocatorPolicy::Last) + 1;

    constexpr size_t STACK_COUNT = static_cast<size_t>(StackID::Last) + 1;

    namespace pisdf {
        constexpr auto SPECIAL_VERTEX_COUNT =
                static_cast<uint_least8_t>(VertexType::END) - static_cast<uint_least8_t>(VertexType::CONFIG) + 1;
        constexpr auto SPECIAL_KERNEL_COUNT =
                static_cast<uint_least8_t>(VertexType::END) - static_cast<uint_least8_t>(VertexType::FORK) + 1;
        constexpr auto VERTEX_TYPE_COUNT =
                static_cast<uint_least8_t>(VertexType::Last) - static_cast<uint_least8_t>(VertexType::First) + 1;
    }

    /* === Const array(s) === */

    inline std::array<const char *, STACK_COUNT> &stackNamesArray() {
        static std::array<const char *, STACK_COUNT> nameArray = {{ "pisdf-stack",
                                                                          "archi-stack",
                                                                          "transfo-stack",
                                                                          "optims-stack",
                                                                          "expr-stack",
                                                                          "sched-stack",
                                                                          "runtime-stack",
                                                                          "general-stack" }};
        return nameArray;
    }

    inline std::array<Stack *, STACK_COUNT> &stackArray() {
        static std::array<Stack *, STACK_COUNT> stackArray = {{ nullptr }};
        return stackArray;
    }

    /* === Type definition(s) === */

    /**
     * @brief Memory exchange cost routine (overridable).
     */
    using MemoryExchangeCostRoutine = uint_least64_t (*)(uint_least64_t /* = Number of bytes = */);

    /**
     * @brief Memory bus send / receive routine.
     */
    using MemoryBusRoutine = void (*)(int_least64_t  /* = Size in bytes = */,
                                      int_least32_t, /* = Packet id = */
                                      void *         /* = Buffer to send / receive = */);

    /**
     * @brief Data memory allocation routine (overridable).
     *        This should return the allocated buffer.
     */
    using MemoryAllocateRoutine = void *(*)(uint_least64_t /* = Number of bytes = */);

    /**
     * @brief Data memory deallocation routine (overridable).
     */
    using MemoryDeallocateRoutine = void (*)(void * /* = physical address to free = */);

    /**
     * @brief Generic refinement used by spider for the actors.
     */
    using Kernel = void (*)(const int_least64_t *, int_least64_t *, void *[], void *[]);
}

#endif //SPIDER2_GLOBAL_API_H
