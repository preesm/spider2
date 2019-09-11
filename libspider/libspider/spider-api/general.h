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
#ifndef SPIDER2_GENERAL_H
#define SPIDER2_GENERAL_H

/* === Includes === */

/* === Forward declaration(s) === */

class PiSDFGraph;

/* === Define(s) === */

#define NB_ALLOCATORS 7

/* === Enumeration(s) === */

/**
 * @brief Stack ids
 */
enum class StackID : std::uint64_t {
    PISDF = 0,         /*!< Stack used for PISDF graph (should be static) */
    ARCHI = 1,         /*!< Stack used for architecture (should be static) */
    TRANSFO = 2,       /*!< Stack used for graph transformations */
    SCHEDULE = 3,      /*!< Stack used for scheduling */
    LRT = 4,           /*!< Stack used by LRTs */
    EXPR_PARSER = 5,   /*!< Stack used by the expression parser */
    GENERAL = 6,       /*!< General stack used for classic new / delete */
};

/**
 * @brief Allocator types
 */
enum class AllocatorType {
    FREELIST,        /*!< (Dynamic) FreeList type allocator */
    GENERIC,         /*!< (Dynamic) Generic type allocator (=malloc) */
    LIFO_STATIC,     /*!< (Static) LIFO type allocator */
    FREELIST_STATIC, /*!< (Static) FreeList type allocator */
    LINEAR_STATIC    /*!< (Static) Linear type allocator */
};

/* === Forward declaration(s) === */

enum class FreeListPolicy;

/* === Methods prototype === */

namespace Spider {
    namespace API {

        /**
         * @brief Initialize a given stack.
         * @param stackId   Id of the stack to init (see @refitem StackID)
         * @param name      Name to set to the stack.
         * @param type      Type of memory allocator used for the stack (see @refitem AllocatorType)
         * @param size      Size of the memory stack (size of the base static memory for non-static allocators)
         * @param baseAddr  Base address provided by user instead of the default one. (for static allocators only)
         * @param alignment Alignment base used (default alignment is sizeof(std::uint64_t) = 64bits).
         */
        void initStack(StackID stackId, const std::string &name, AllocatorType type, std::uint64_t size,
                       char *baseAddr = nullptr,
                       std::uint64_t alignment = sizeof(std::uint64_t));

        /**
         * @brief Initialize a given stack (specialized version for FreeList and FreeListStatic allocators).
         * @param stackId   Id of the stack to init (see @refitem StackID)
         * @param name      Name to set to the stack.
         * @param type      Type of memory allocator used for the stack (see @refitem AllocatorType)
         * @param size      Size of the memory stack (size of the base static memory for non-static allocators)
         * @param policy    Policy of search of the memory block in the FreeList allocators (default is FIND_FIRST).
         * @param baseAddr  Base address provided by user instead of the default one. (for static allocators only)
         * @param alignment Alignment base used (default alignment is sizeof(std::uint64_t) = 64bits).
         */
        void
        initStack(StackID stackId, const std::string &name, AllocatorType type, std::uint64_t size,
                  FreeListPolicy policy,
                  char *baseAddr = nullptr, std::uint64_t alignment = sizeof(std::uint64_t));

        void start();

        void quit();

        /* === Methods for setting flags value in Spider === */

        /**
         * @brief Enable the traces in Spider.
         */
        void enableTrace();

        /**
         * @brief Disable the traces in Spider (default behavior).
         */
        void disableTrace();

        /**
         * @brief Enable the Verbose mode in Spider.
         */
        void enableVerbose();

        /**
         * @brief Disable the Verbose mode in Spider (default behavior).
         */
        void disableVerbose();

        /**
         * @brief Enable the Logs related to the JOBs.
         */
        void enableJobLogs();

        /**
         * @brief Disable the Logs related to the JOBs (default behavior).
         */
        void disableJobLogs();

        /**
         * @brief Enable the schedule optimization for static graphs (default behavior).
         */
        void enableStaticScheduleOptim();

        /**
         * @brief Disable the schedule optimization for static graphs (schedule will be recomputed every iteration).
         */
        void disableStaticScheduleOptim();

        /**
         * @brief Enable the SRDAG optimizations (default behavior).
         */
        void enableSRDAGOptims();

        /**
         * @brief Disable the SRDAG optimizations.
         */
        void disableSRDAGOptims();

        /* === Getters for static variables === */

        /**
         * @brief Get the trace flag value.
         * @return value of trace flag.
         */
        bool trace();

        /**
         * @brief Get the verbose flag value.
         * @return value of verbose flag.
         */
        bool verbose();

        /**
         * @brief Get the staticOptims flag value.
         * @return value of staticOptims flag.
         */
        bool staticOptim();

        /**
         * @brief Get the srdagOptim flag value.
         * @return value of srdagOptim flag.
         */
        bool srdagOptim();
    }
}

#endif //SPIDER2_GENERAL_H
