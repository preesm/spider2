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
#ifndef SPIDER2_RTINFO_H
#define SPIDER2_RTINFO_H

/* === Include(s) === */

#include <graphs-tools/expression-parser/Expression.h>
#include <archi/Platform.h>
#include <api/archi-api.h>
#include <archi/Cluster.h>
#include <archi/PE.h>

namespace spider {

    /* === Class definition === */

    class RTInfo {
    public:

        RTInfo() {
            const auto *platform = archi::platform();
            if (platform) {
                peMappableArray_ = spider::make_n<bool, StackID::RUNTIME>(platform->PECount(), true);
                clusterMappableArray_ = spider::make_n<bool, StackID::RUNTIME>(platform->clusterCount(), true);
                timingArray_ = spider::make_n<Expression, StackID::RUNTIME>(platform->HWTypeCount(), Expression(100));
            }
        }

        RTInfo(const RTInfo &) = default;

        RTInfo(RTInfo &&) = default;

        RTInfo &operator=(const RTInfo &) = default;

        RTInfo &operator=(RTInfo &&) = default;

        ~RTInfo() {
            if (archi::platform()) {
                deallocate(peMappableArray_);
                deallocate(clusterMappableArray_);
                for (size_t i = 0; i < archi::platform()->HWTypeCount(); ++i) {
                    timingArray_[i].~Expression();
                }
                deallocate(timingArray_);
            }
        }

        /* === Getter(s) === */

        /**
         * @brief Evaluate if vertex associated to this RTConstraints is mappable at PE level.
         * @param pe PE to evaluate.
         * @return true if mappable, false else. If pe is nullptr, return false.
         * @throws std::out_of_range
         */
        inline bool isPEMappable(const PE *pe) const {
            if (!pe) {
                return false;
            }
            return peMappableArray_[pe->virtualIx()];
        }

        /**
         * @brief Evaluate if vertex associated to this RTConstraints is mappable at cluster level.
         * @param cluster Cluster to evaluate.
         * @return true if mappable, false else. If cluster is nullptr, return false.
         * @throws std::out_of_range
         */
        inline bool isClusterMappable(const Cluster *cluster) const {
            if (!cluster) {
                return false;
            }
            return clusterMappableArray_[cluster->ix()];
        }

        /**
         * @brief Evaluate the timing of vertex associated to this RTConstraints on given PE.
         * @param pe PE to evaluate.
         * @param params  Extra parameters in case timing is parameterized.
         * @return timing on given PE (100 by default). If pe is nullptr, return INT64_MAX.
         * @throws std::out_of_range
         */
        inline int64_t
        timingOnPE(const PE *pe, const spider::vector<std::shared_ptr<pisdf::Param>> &params = { }) const {
            if (!pe) {
                return INT64_MAX;
            }
            return timingArray_[pe->hardwareType()].evaluate(params);
        }

        /**
         * @brief Evaluate the timing of vertex associated to this RTConstraints on given PE.
         * @param ix      Spider pe ix to evaluate.
         * @param params  Extra parameters in case timing is parameterized.
         * @return timing on given PE (100 by default).
         * @throws std::out_of_range
         */
        inline int64_t timingOnPE(size_t ix, const spider::vector<std::shared_ptr<pisdf::Param>> &params = { }) const {
            const auto *pe = archi::platform()->processingElement(ix);
            return timingArray_[pe->hardwareType()].evaluate(params);
        }

        /**
         * @brief Evaluate if vertex associated to this RTConstraints is mappable at PE level.
         * @param ix Spider pe ix to evaluate.
         * @return true if mappable, false else.
         * @throws std::out_of_range
         */
        inline bool isPEMappable(size_t ix) const {
#ifndef NDEBUG
            if (ix >= archi::platform()->PECount()) {
                throwSpiderException("index out of bound.");
            }
#endif
            return peMappableArray_[ix];
        }

        /**
         * @brief Evaluate if vertex associated to this RTConstraints is mappable at cluster level.
         * @param ix Spider cluster ix to evaluate.
         * @return true if mappable, false else.
         * @throws std::out_of_range
         */
        inline bool isClusterMappable(size_t ix) const {
            return clusterMappableArray_[ix];
        }

        /**
         * @brief Get the index of the kernel in the global register associated to this vertex.
         * @return index of the refinement, UINT32_MAX if not set.
         */
        inline size_t kernelIx() const {
            return kernelIx_;
        }

        /* === Setter(s) === */

        /**
         * @brief Set mappable constraint on given PE.
         * @remark This method update value of clusterMappableVector_ of the corresponding cluster.
         * @param pe        PE to evaluate.
         * @param mappable  Value to set (default is true).
         * @throws std::out_of_range
         */
        inline void setMappableConstraintOnPE(const PE *pe, bool mappable = true) {
            const auto *cluster = pe->cluster();
            peMappableArray_[pe->virtualIx()] = mappable;
            if (!mappable) {
                /* == We need to recompute the condition to check if at least one PE in the cluster is valid == */
                bool clusterMappable = false;
                for (const auto *elt : cluster->peArray()) {
                    clusterMappable |= peMappableArray_[elt->virtualIx()];
                }
                clusterMappableArray_[cluster->ix()] = clusterMappable;
            } else {
                clusterMappableArray_[cluster->ix()] = mappable;
            }
        }

        /**
         * @brief Set mappable constraint on all PE.
         * @param mappable  Value to set (default is true).
         */
        inline void setMappableConstraintOnAllPE(bool mappable = true) {
            if (archi::platform()) {
                std::fill(peMappableArray_, peMappableArray_ + archi::platform()->PECount(), mappable);
                std::fill(clusterMappableArray_, clusterMappableArray_ + archi::platform()->clusterCount(), mappable);
            }
        }

        /**
         * @brief Set timing on a given hardware type by value.
         * @param hardwareType Hardware type to set the timing for (ex: x86, ARM, etc.).
         * @param value        Value to set (default is 100).
         * @throws std::out_of_range
         */
        inline void setTimingOnHWType(u32 hardwareType, int64_t value = 100) {
#ifndef NDEBUG
            if (hardwareType >= archi::platform()->HWTypeCount()) {
                throwSpiderException("index out of bound.");
            }
#endif
            timingArray_[hardwareType] = Expression(value);
        }

        /**
         * @brief Set timing on a given hardware type by Expression.
         * @param hardwareType Hardware type to set the timing for (ex: x86, ARM, etc.).
         * @param expression   Expression to set.
         * @throws std::out_of_range
         */
        inline void setTimingOnHWType(u32 hardwareType, Expression expression) {
#ifndef NDEBUG
            if (hardwareType >= archi::platform()->HWTypeCount()) {
                throwSpiderException("index out of bound.");
            }
#endif
            timingArray_[hardwareType] = std::move(expression);
        }

        /**
         * @brief Set timing on a given cluster by value.
         * @param ix     Spider cluster ix to evaluate.
         * @param value  Value to set (default is 100).
         * @throws std::out_of_range
         */
        inline void setTimingOnCluster(size_t ix, int64_t value = 100) {
#ifndef NDEBUG
            if (ix >= archi::platform()->HWTypeCount()) {
                throwSpiderException("index out of bound.");
            }
#endif
            timingArray_[ix] = Expression(value);
        }

        /**
         * @brief Set timing on a given cluster by Expression.
         * @param ix          Spider cluster ix to evaluate.
         * @param expression  Expression to set.
         */
        inline void setTimingOnCluster(size_t ix, Expression expression) {
#ifndef NDEBUG
            if (ix >= archi::platform()->HWTypeCount()) {
                throwSpiderException("index out of bound.");
            }
#endif
            timingArray_[ix] = std::move(expression);
        }

        /**
         * @brief Set timing on all Cluster by value.
         * @param value  Value to set (default is 100).
         */
        inline void setTimingOnAllHWTypes(int64_t value = 100) {
            if (archi::platform()) {
                std::fill(timingArray_, timingArray_ + archi::platform()->HWTypeCount(), Expression(value));
            }
        }

        /**
         * @brief Set timing on all Cluster by Expression.
         * @param expression Expression to set.
         */
        inline void setTimingOnAllHWTypes(const Expression &expression) {
            if (archi::platform()) {
                std::fill(timingArray_, timingArray_ + archi::platform()->HWTypeCount(), expression);
            }
        }

        /**
         * @brief Set the index of the kernel associated to the vertex.
         * @param ix  Index to set.
         */
        inline void setKernelIx(size_t ix) {
            kernelIx_ = ix;
        }

    private:
        bool *peMappableArray_ = nullptr;
        bool *clusterMappableArray_ = nullptr;
        Expression *timingArray_ = nullptr;
        size_t kernelIx_ = SIZE_MAX;
    };
}
#endif //SPIDER2_RTINFO_H
