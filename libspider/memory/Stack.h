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
#ifndef SPIDER2_STACK_H
#define SPIDER2_STACK_H

/* === Include(s) === */

#include <string>
#include <memory/abstract-policies/AbstractAllocatorPolicy.h>
#include <memory/dynamic-policies/GenericAllocatorPolicy.h>
#include <api/global-api.h>
#include <cmath>

namespace spider {

    /* === Class definition === */

    class Stack {
    public:
        explicit Stack(StackID stack) : stack_{ stack } {
            policy_ = new GenericAllocatorPolicy();
        }

        Stack(Stack &&) = delete;

        Stack(const Stack &) = delete;

        Stack &operator=(Stack &&) = delete;

        Stack &operator=(const Stack &) = delete;

        ~Stack() {
            print(stackNamesArray()[static_cast<uint64_t>(stack_)], peak_, total_, sampleCount_, usage_);
            delete policy_;
        }

        /* === Method(s) === */

        inline void increaseUsage(uint64_t size) {
            usage_ += size;
            peak_ = std::max(peak_, usage_);
            total_ += usage_;
            sampleCount_++;
        }

        inline void decreaseUsage(uint64_t size) {
            usage_ -= size;
        }

        inline static void
        print(const char *name, uint64_t peak, uint64_t total, uint64_t sampleCount, uint64_t usage) {
            // TODO: get stack name
            if (peak && log::enabled()) {
                const auto &average = total / sampleCount;
                const auto &normalizedPeak = getByteNormalizedSize(peak);
                const auto &normalizedAverage = getByteNormalizedSize(average);
                const auto &normalizedUsage = getByteNormalizedSize(usage);
                const auto &maxWidth = computeMaxWidth(normalizedPeak, normalizedAverage, normalizedUsage);
                log::info("---------------------------\n");
                log::info("Stack: %s\n", name);
                auto unitPeak = std::string(maxWidth - getWidth(normalizedPeak), ' ') +
                                getByteUnitString(peak);
                unitPeak = unitPeak.size() > 1 ? unitPeak : " " + unitPeak;
                log::info("        ==>    peak: %.1lf %s (%" PRIu64" B)\n", normalizedPeak,
                          unitPeak.c_str(),
                          peak);
                auto unitAverage = std::string(maxWidth - getWidth(normalizedAverage), ' ') +
                                   std::string(getByteUnitString(average));
                unitAverage = unitAverage.size() > 1 ? unitAverage : " " + unitAverage;
                log::info("        ==> average: %.1lf %s (%" PRIu64" B)\n", normalizedAverage,
                          unitAverage.c_str(),
                          average);
                if (usage) {
                    auto unitUsage = std::string(maxWidth - getWidth(normalizedUsage), ' ') +
                                     std::string(getByteUnitString(usage));
                    unitUsage = unitUsage.size() > 1 ? unitUsage : " " + unitUsage;
                    log::error("         ==>  in-use: %.1lf %s (%" PRIu64" B)\n", normalizedUsage,
                               unitUsage.c_str(),
                               usage);
                }
                log::info("---------------------------\n");
            }
        }

        inline void *allocate(size_t size) {
            auto allocResult = policy_->allocate(size);
            increaseUsage(allocResult.second);
            return allocResult.first;
        }

        inline void deallocate(void *ptr) {
            decreaseUsage(policy_->deallocate(ptr));
        }

        /* === Getter(s) === */

        inline AbstractAllocatorPolicy *policy() const {
            return policy_;
        }

        inline uint64_t peak() const {
            return peak_;
        }

        inline uint64_t usage() const {
            return usage_;
        }

        inline uint64_t average() const {
            if (sampleCount_) {
                return total_ / sampleCount_;
            }
            return 0;
        }

        /* === Setter(s) === */

        /**
         * @brief Set the allocation policy of the stack.
         * @remark If policy is nullptr or current policy has still memory in-use, nothing happens.
         * @warning This is non-thread safe, change of policy should be done at quiescent points.
         * @param policy Pointer to the policy to set.
         * @return true if operation succeed, false otherwise.
         */
        inline bool setPolicy(AbstractAllocatorPolicy *policy) {
            if (!policy || policy_->usage()) {
                return false;
            }
            delete policy_;
            policy_ = policy;
            return true;
        }

    private:
        uint64_t usage_ = 0;
        uint64_t peak_ = 0;
        uint64_t total_ = 0;
        uint64_t sampleCount_ = 0;
        StackID stack_ = StackID::GENERAL;
        AbstractAllocatorPolicy *policy_ = nullptr;


        /* === Private methods === */

        static inline const char *getByteUnitString(uint64_t size) noexcept {
            constexpr uint64_t SIZE_GB = 1024 * 1024 * 1024;
            constexpr uint64_t SIZE_MB = 1024 * 1024;
            constexpr uint64_t SIZE_KB = 1024;
            if (size / SIZE_GB) {
                return "GB";
            } else if (size / SIZE_MB) {
                return "MB";
            } else if (size / SIZE_KB) {
                return "KB";
            }
            return "B";
        }

        static inline double getByteNormalizedSize(uint64_t size) noexcept {
            constexpr double SIZE_GB = 1024 * 1024 * 1024;
            constexpr double SIZE_MB = 1024 * 1024;
            constexpr double SIZE_KB = 1024;
            const auto dblSize = (double) size;
            if (dblSize / SIZE_GB >= 1.) {
                return dblSize / SIZE_GB;
            } else if (dblSize / SIZE_MB >= 1.) {
                return dblSize / SIZE_MB;
            } else if (dblSize / SIZE_KB >= 1.) {
                return dblSize / SIZE_KB;
            }
            return dblSize;
        }

        static inline uint32_t computeMaxWidth(double peak, double average, double usage) {
            const auto &widthPeak = getWidth(peak);
            const auto &widthAverage = getWidth(average);
            const auto &widthUsage = getWidth(usage);
            return std::max(std::max(widthPeak, widthAverage), widthUsage);
        }

        static inline uint32_t getWidth(double value) {
            /* == We add two because printing format is .1f so there are two additional char == */
            return (static_cast<uint32_t>(std::log10(value)) + 1) + 2;
        }
    };
}


#endif //SPIDER2_STACK_H
