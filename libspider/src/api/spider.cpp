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
/* === Includes === */

#include <api/spider.h>
#include <memory/Stack.h>
#include <memory/memory.h>
#include <common/Logger.h>
#include <archi/Platform.h>
#include <runtime/platform/RTPlatform.h>
#include <graphs/pisdf/Graph.h>
#include <runtime/algorithm/Runtime.h>
#include <runtime/algorithm/JITMSRuntime.h>
#include <runtime/algorithm/FastJITMSRuntime.h>

/* === Static variable(s) definition === */

static bool startFlag = false;
extern bool spider2StopRunning;

/* === Static function(s) === */

static const char *printFlagStatus(bool flag) {
    return flag ? "ENABLED" : "DISABLED";
}

static void printSpiderStartUpLogo() {
    spider::printer::fprintf(stderr, "\n");
    spider::printer::fprintf(stderr, "  .;;;;;;;  ==========================================  ;;;;;;;.  \n");
    spider::printer::fprintf(stderr, " ;;;;;;;;;          SPIDER 2.0 Runtime Library          ;;;;;;;;; \n");
    spider::printer::fprintf(stderr, ";;;;;;;;;;  ==========================================  ;;;;;;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;             ;8.                        :@.             ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;               ,@8                   .@@                ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                 L@8                @@:                 ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                  .@@;            C@@                   ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                    @@0          @@@                    ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                    .@@0        @@@                     ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                     L@@1      8@@:                     ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                      @@@      @@@                      ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;     :L@@@@@@@@t      f@@      @@,      C@@@@@@@8t,     ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;            :0@@@@@G   @@     .@@   8@@@@@C.            ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                 ;@@@1 @@     :@8 G@@@.                 ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                   L@@f8@@@@@@8@t8@@:                   ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                     @@i@@@@@@@8G@8                     ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;      t@@@@@@@@@@@@G. L@@@@@@@@@@; ,8@@@@@@@@@@@@;      ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                  C@@@@L@@@@@@@C8@@@@t                  ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                       :,@@@@@0:,                       ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                      t@@@@@@@@@@:                      ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                   .@@@8@@@@@@@@@@@C                    ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                  1@@@ @@@0ii0@@f.@@@.                  ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                 C@@C  @@,@8G@,@f  @@@;                 ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                i@@1   ;@8.  ,8@    0@@                 ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;                @@:     ;@@@@@@.     C@8                ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;               i@.         ,,         1@                ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;               0                        @               ;;;;;\n");
    spider::printer::fprintf(stderr, ";;;;;;;;;;                                              ;;;;;;;;;;\n");
    spider::printer::fprintf(stderr, " ;;;;;;;;;                                              ;;;;;;;;; \n");
    spider::printer::fprintf(stderr, "  .;;;;;;;                                              ;;;;;;;.  \n");
    spider::printer::fprintf(stderr, "\n");
}

static void printConfig(const spider::StartUpConfig &cfg) {
    printSpiderStartUpLogo();
    spider::printer::fprintf(stderr, "==============================\n");
    spider::printer::fprintf(stderr, " Start-up configuration:\n");
    spider::printer::fprintf(stderr, "      verbose:        %s\n", printFlagStatus(cfg.verbose_));
    spider::printer::fprintf(stderr, "      papify:         %s\n", printFlagStatus(cfg.usePapify_));
    spider::printer::fprintf(stderr, "      apollo:         %s\n", printFlagStatus(cfg.useApollo_));
    spider::printer::fprintf(stderr, "      general-log:    %s\n", printFlagStatus(cfg.enableGeneralLog_));
    spider::printer::fprintf(stderr, "      stand-alone:    %s\n", printFlagStatus(cfg.standAlone_));
    spider::printer::fprintf(stderr, "      export-trace:   %s\n", printFlagStatus(cfg.exportTrace_));
    spider::printer::fprintf(stderr, "      export-srdag:   %s\n", printFlagStatus(cfg.exportSRDAG_));
    if (cfg.standAlone_) {
        spider::printer::fprintf(stderr, "      stand-alone ix: %zu\n", cfg.standAloneClusterIx_);
    }
    spider::printer::fprintf(stderr, "==============================\n");
}

/* === Function(s) definition === */

spider::StartUpConfig spider::parseInputArguments(int32_t argc, char **argv) {
    spider::log::info("parsing of input arguments is not yet supported.\n");
    for (auto i = 0; i < argc; ++i) {
        spider::log::info("argv[%d]: %s\n", i, argv[i]);
    }
    return { };
}

void spider::api::setStackAllocatorPolicy(StackID stackId,
                                          AllocatorPolicy policy,
                                          size_t alignment,
                                          size_t size,
                                          void *externBuffer) {
    auto *stack = stackArray()[static_cast<size_t>(stackId)];
    switch (policy) {
        case AllocatorPolicy::FREELIST_FIND_FIRST:
            stack->setPolicy(new FreeListAllocatorPolicy(size, externBuffer, FreeListPolicy::FIND_FIRST, alignment));
            break;
        case AllocatorPolicy::FREELIST_FIND_BEST:
            stack->setPolicy(new FreeListAllocatorPolicy(size, externBuffer, FreeListPolicy::FIND_BEST, alignment));
            break;
        case AllocatorPolicy::GENERIC:
            stack->setPolicy(new GenericAllocatorPolicy(alignment));
            break;
        case AllocatorPolicy::LINEAR_STATIC:
            stack->setPolicy(new LinearStaticAllocator(size, externBuffer, alignment));
            break;
    }
}

void spider::start(const StartUpConfig &cfg) {
    if (startFlag) {
        throwSpiderException("spider::start() function should be called only once.");
    }

    /* == Print the configuration == */
    printConfig(cfg);

    /* == Initialize stacks == */
    auto it = EnumIterator<StackID>{ }.begin();
    for (auto &stack : stackArray()) {
        stack = new Stack(*(it++));
    }
    if (cfg.generalStackAllocatorPolicy_ != AllocatorPolicy::GENERIC) {
        api::setStackAllocatorPolicy(StackID::GENERAL,
                                     cfg.generalStackAllocatorPolicy_,
                                     cfg.generalStackAlignment_,
                                     cfg.generalStackSize_,
                                     cfg.generalStackExternAddress_);
    }

    /* == Init the Logger and enable the GENERAL Logger == */
    if (cfg.enableGeneralLog_) {
        log::enable<log::GENERAL>();
    }

    /* == Enable the verbose == */
    if (cfg.verbose_) {
        api::enableVerbose();
    }

    /* == Enable export trace == */
    if (cfg.exportTrace_) {
        api::enableExportTrace();
    }

    /* == Enable export SRDAG == */
    if (cfg.exportSRDAG_) {
        api::enableExportSRDAG();
    }

    /* == Enable the config flag == */
    startFlag = true;
}

bool spider::isInit() {
    return startFlag;
}

static spider::Runtime *getRuntimeFromType(spider::pisdf::Graph *graph,
                                           spider::RuntimeType type,
                                           spider::SchedulingPolicy policy) {
    switch (type) {
        case spider::RuntimeType::JITMS:
            return spider::make<spider::JITMSRuntime>(StackID::GENERAL, graph, policy);
        case spider::RuntimeType::FAST_JITMS:
            return spider::make<spider::FastJITMSRuntime>(StackID::GENERAL, graph, policy);
        default:
            return nullptr;
    }
}


spider::RuntimeContext
spider::createRuntimeContext(pisdf::Graph *graph, RunMode mode, size_t loopCount, RuntimeType type,
                             SchedulingPolicy policy) {
    if (!isInit()) {
        log::warning("SPIDER has not been initialized, returning.\n");
        return RuntimeContext{ };
    }
    RuntimeContext context{ };
    context.graph_ = graph;
    context.algorithm_ = getRuntimeFromType(graph, type, policy);
    if (!context.algorithm_) {
        throwSpiderException("could not create runtime algorithm.");
    }
    context.loopSize_ = loopCount;
    context.mode_ = mode;
    return context;
}

void spider::run(spider::RuntimeContext &context) {
    try {
        switch (context.mode_) {
            case RunMode::INFINITE:
                while (!spider2StopRunning) {
                    context.algorithm_->execute();
                }
                break;
            case RunMode::LOOP:
                for (size_t i = 0; (i < context.loopSize_) && !spider2StopRunning; ++i) {
                    context.algorithm_->execute();
                }
                break;
            case RunMode::EXTERN_LOOP:
                context.algorithm_->execute();
                break;
            default:
                break;
        }
    } catch (spider::Exception &e) {
        throw std::runtime_error(e.what());
    }
}

void spider::destroyRuntimeContext(RuntimeContext &context) {
    context.graph_ = nullptr;
    destroy(context.algorithm_);
    context.loopSize_ = 0;
    context.mode_ = RunMode::LOOP;
}

void spider::quit() {
    if (!isInit()) {
        log::warning("SPIDER has not been initialized, returning.\n");
        return;
    }

    /* == Destroy the runtime Platform == */
    destroy(rt::platform());

    /* == Destroy the Platform == */
    destroy(archi::platform());

    /* == Clear the stacks == */
    uint64_t totalUsage = 0;
    uint64_t totalAverage = 0;
    uint64_t totalPeak = 0;
    for (auto &stack : stackArray()) {
        if (stack) {
            totalUsage += stack->usage();
            totalAverage += stack->average();
            totalPeak += stack->peak();
        }
        delete stack;
    }
    Stack::print("Total", totalPeak, totalAverage, 1, totalUsage);

    /* == Reset start flag == */
    startFlag = false;
}
