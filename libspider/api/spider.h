/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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
#ifndef SPIDER2_SPIDER_H
#define SPIDER2_SPIDER_H

/* === Includes === */

#include <api/archi-api.h>
#include <api/debug-api.h>
#include <api/config-api.h>
#include <api/pisdf-api.h>
#include <api/runtime-api.h>
#include <api/global-api.h>

/* === Methods prototype === */

namespace spider {

    struct StartUpConfig {
        bool verbose_ = false;            /* = Enable / disable the verbose = */
        bool standAlone_ = false;         /* = Enable / disable the stand-alone mode = */
        bool usePapify_ = false;          /* = Enable / disable the papify support (if available) = */
        bool useApollo_ = false;          /* = Enable / disable the apollo support (if available) = */
        bool enableGeneralLog_ = true;    /* = Enable / disable the main logger = */
        bool exportTrace_ = false;        /* = Enable / disable the export of the traces = */
        bool exportSRDAG_ = false;        /* = Enable / disable the export of the srdag = */
        bool adaptiveStaticSched_ = true; /* = Enable / disable the adaptive static scheduling method = */
        size_t standAloneClusterIx_ = SIZE_MAX; /* = Id of the current cluster in stand-alone mode = */
        AllocatorPolicy generalStackAllocatorPolicy_ = AllocatorPolicy::GENERIC; /* = Allocation policy of the general stack = */
        size_t generalStackAlignment_ = sizeof(int64_t);
        size_t generalStackSize_ = SIZE_MAX;
        void *generalStackExternAddress_ = nullptr;
    };

    struct RuntimeContext {
        pisdf::Graph *graph_ = nullptr;
        Runtime *algorithm_ = nullptr;
        size_t loopSize_ = 0;
        RunMode mode_ = RunMode::LOOP;
    };

    struct RuntimeConfig {
        RunMode mode_ = RunMode::LOOP;                             /*!< Execution mode: default is LOOP */
        RuntimeType runtimeType_ = RuntimeType::JITMS;             /*!< Runtime algorithm to use: default is JITMS */
        ExecutionPolicy execPolicy_ = ExecutionPolicy::DELAYED;    /*!< Execution policy to use: default is DELAYED */
        SchedulingPolicy schedPolicy_ = SchedulingPolicy::LIST;    /*!< Scheduling policy to use: default is LIST */
        MappingPolicy mapPolicy_ = MappingPolicy::BEST_FIT;        /*!< Mapping policy to use: default is BEST_FIT */
        FifoAllocatorType allocType_ = FifoAllocatorType::DEFAULT; /*!< Allocator type to use */
        size_t loopCount_ = 1000U;                                 /*!< Number of loop to perform (only used in LOOP mode) */

        RuntimeConfig() = default;

        RuntimeConfig(const RuntimeConfig &) = default;

        RuntimeConfig(RuntimeConfig &&) = default;

        explicit RuntimeConfig(RunMode mode = RunMode::LOOP,
                               RuntimeType type = RuntimeType::JITMS,
                               ExecutionPolicy execPolicy = ExecutionPolicy::DELAYED,
                               SchedulingPolicy schedPolicy = SchedulingPolicy::LIST,
                               MappingPolicy mapPolicy = MappingPolicy::BEST_FIT,
                               FifoAllocatorType allocatorType = FifoAllocatorType::DEFAULT,
                               size_t loopCount = 0U) : mode_{ mode },
                                                        runtimeType_{ type },
                                                        execPolicy_{ execPolicy },
                                                        schedPolicy_{ schedPolicy },
                                                        mapPolicy_{ mapPolicy },
                                                        allocType_{ allocatorType },
                                                        loopCount_{ loopCount } { };

        explicit RuntimeConfig(RunMode mode) : mode_{ mode } { };

        explicit RuntimeConfig(RuntimeType type) : runtimeType_{ type } { };

        explicit RuntimeConfig(ExecutionPolicy policy) : execPolicy_{ policy } { };

        explicit RuntimeConfig(SchedulingPolicy policy) : schedPolicy_{ policy } { };

        explicit RuntimeConfig(MappingPolicy policy) : mapPolicy_{ policy } { };

        explicit RuntimeConfig(FifoAllocatorType type) : allocType_{ type } { };

        explicit RuntimeConfig(size_t loopCount) : loopCount_{ loopCount } { };

    };

    /**
     * @brief Parse program input arguments and intiliaze @refitem StartUpConfig accordingly.
     * @remark On error, it will print the usage and exit the program.
     * @param argc Number of input arguments.
     * @param argv Array of input arguments.
     * @return Initialized @refitem StartUpConfig.
     */
    StartUpConfig parseInputArguments(int32_t argc, char *argv[]);


    /**
     * @brief Function to be called before any other function of the spider runtime.
     * @param cfg  Start-up configuration tto be used on startup by spider.
     */
    void start(const StartUpConfig &cfg = StartUpConfig());

    /**
     * @brief Check if SPIDER runtime is already initialized.
     * @return true if spider is initialized, false else.
     */
    bool isInit();

    /**
     * @brief Creates a runtime context for a given graph according to the given config.
     * @remark In INFINITE mode, the application can only be stopped properly on receive of the SIGINT signal.
     * @warning If the user application already catches this signal, it should define and set the extern bool
     *          variable spider2StopRunning to true.
     * @param graph   Pointer to the graph associated to this context.
     * @param config  Config of the runtime context (see @refitem RuntimeConfig).
     * @return  Created runtime context.
     */
    RuntimeContext createRuntimeContext(pisdf::Graph *graph, RuntimeConfig config);

    /**
     * @brief Run a given runtime context.
     * @param context Context to run.
     */
    void run(RuntimeContext &context);

    /**
     * @brief Destroy a runtime context.
     * @param context Reference to the context to destroy.
     */
    void destroyRuntimeContext(RuntimeContext &context);

    /**
     * @brief Function to call at the end of the application using Spider to close correctly the runtime.
     */
    void quit();

    namespace api {

        void setStackAllocatorPolicy(StackID stackId,
                                     AllocatorPolicy policy,
                                     size_t alignment = sizeof(uint64_t),
                                     size_t size = 0,
                                     void *externBuffer = nullptr);
    }

}

#endif //SPIDER2_SPIDER_H
