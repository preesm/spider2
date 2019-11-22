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
#ifndef SPIDER2_SCENARIO_H
#define SPIDER2_SCENARIO_H

/* === Include(s) === */

#include <cstdint>
#include <graphs-tools/expression-parser/Expression.h>
#include <containers/containers.h>

namespace spider {

    /* === Forward declaration(s) === */

    class PE;

    namespace pisdf {

        class Vertex;

        struct GraphAddVertexVisitor;

        struct GraphRemoveVertexVisitor;
    }

    /* === Class definition === */

    class Scenario {
    public:

        Scenario() = default;

        Scenario(const Scenario &) = default;

        Scenario(Scenario &&) = default;

        ~Scenario() = default;

        friend spider::pisdf::GraphAddVertexVisitor;

        friend spider::pisdf::GraphRemoveVertexVisitor;

        /* === Method(s) === */

        /* === Getter(s) === */

        /**
         * @brief Get the mappable constraints of a vertex for all PE in the platform.
         * @param vertex  Vertex to evaluate.
         * @return mapping constraints vector.
         * @throws @refitem std::out_of_range if vertex does not exist in the lookup table.
         */
        const std::vector<bool> &mappingConstraints(const spider::pisdf::Vertex *vertex) const;

        /**
         * @brief Get the execution timings of a given vertex for all PE type in the platform.
         * @param vertex   Vertex to evaluate.
         * @return execution timings vector.
         * @throws @refitem std::out_of_range if vertex does not exist in the lookup table.
         */
        const spider::vector<Expression> &executionTimings(const spider::pisdf::Vertex *vertex) const;

        /**
         * @brief Get the mappable constraint of a vertex on a given PE.
         * @param vertex  Vertex to evaluate.
         * @param PE      PE to evaluate.
         * @return true if vertex is mappable on PE, false else.
         * @throws @refitem std::out_of_range if vertex or PE do not exist in the lookup table.
         */
        bool isMappable(const spider::pisdf::Vertex *vertex, const PE *PE) const;

        /**
         * @brief Get the execution timing of a given vertex on a specific PE type.
         * @param vertex   Vertex to evaluate.
         * @param PEType   PE type to evaluate.
         * @return execution timing.
         * @throws @refitem std::out_of_range if vertex or PEType do not exist in the lookup table.
         */
        int64_t executionTiming(const spider::pisdf::Vertex *vertex, uint32_t PEType) const;

        /**
         * @brief Get the execution timing of a given vertex on a specific PE.
         * @remark Timing are not PE specific, i.e all PE of a same PE type share the same timing.
         * @param vertex   Vertex to evaluate.
         * @param PEType   PE to evaluate.
         * @return execution timing.
         * @throws @refitem std::out_of_range if vertex or PEType do not exist in the lookup table.
         */
        int64_t executionTiming(const spider::pisdf::Vertex *vertex, const PE *PE) const;

        /* === Setter(s) === */

        /**
         * @brief Set the mapping constraints of a given vertex for all possible processing elements of the platform.
         * @param vertex        Vertex for which constraints are set.
         * @param constraints   Constraints to set (as initialization list).
         */
        void setMappingConstraints(const spider::pisdf::Vertex *vertex, const std::initializer_list<bool> &constraints);

        /**
         * @brief Set the mapping constraints of a given vertex for all possible processing elements of the platform.
         * @param vertex    Vertex for which constraints are set.
         * @param PECount   Number of PE in the platform.
         * @param value     Constraint value to set (true if mappable, false else).
         */
        void setMappingConstraints(const spider::pisdf::Vertex *vertex, uint32_t PECount, bool value);

        /**
         * @brief Set the mapping constraint of a given vertex for a given processing element of the platform.
         * @param vertex       Vertex for which constraints are set.
         * @param spiderPEIx   Ix of the PE in spider.
         * @param value        Constraint to set (true if mappable, false else)
         */
        void setMappingConstraint(const spider::pisdf::Vertex *vertex, uint32_t spiderPEIx, bool value);

        /**
         * @brief Set the mapping constraint of a given vertex for a given processing element of the platform.
         * @param vertex  Vertex for which constraints are set.
         * @param PE      Processing element.
         * @param value   Constraint to set (true if mappable, false else)
         */
        void setMappingConstraint(const spider::pisdf::Vertex *vertex, const PE *PE, bool value);

        /**
         * @brief Set the execution timings of a given vertex for all possible processing elements of the platform.
         * @param vertex    Vertex for which constraints are set.
         * @param timings   Timings to set (as initialization list).
         */
        void
        setExecutionTimings(const spider::pisdf::Vertex *vertex, const std::initializer_list<int64_t> &timings);

        /**
         * @brief Set the execution timings of a given vertex for all possible processing elements of the platform.
         * @param vertex    Vertex for which constraints are set.
         * @param PETypeCount   Number of PE in the platform.
         * @param value     Constraint value to set (true if mappable, false else).
         */
        void setExecutionTimings(const spider::pisdf::Vertex *vertex, uint32_t PETypeCount, int64_t value);

        /**
         * @brief Set the execution timing of a given vertex for a given processing element type of the platform.
         * @remark This method will replace current value of the timing for all PE sharing the same PEType.
         * @param vertex  Vertex for which timing is set.
         * @param PE      Processing element.
         * @param value   Timing to set.
         */
        void setExecutionTiming(const spider::pisdf::Vertex *vertex, const PE *PE, int64_t value);

        /**
         * @brief Set the execution timing of a given vertex for a given processing element type of the platform.
         * @remark This method will replace current value of the timing for all PE sharing the same PEType.
         * @param vertex     Vertex for which timing is set.
         * @param PE         Processing element.
         * @param expression Expression of the timing to set.
         */
        void setExecutionTiming(const spider::pisdf::Vertex *vertex, const PE *PE, const std::string &expression);

        /**
         * @brief Set the execution timing of a given vertex for a given processing element type of the platform.
         * @remark This method will replace current value of the timing.
         * @param vertex  Vertex for which timing is set.
         * @param PEType  Processing element type (i.e the type of the cluster it belongs. ex: ARM_A7).
         * @param value   Timing to set.
         */
        void setExecutionTiming(const spider::pisdf::Vertex *vertex, uint32_t PEType, int64_t value);

        /**
         * @brief Set the execution timing of a given vertex for a given processing element type of the platform.
         * @remark This method will replace current value of the timing.
         * @param vertex     Vertex for which timing is set.
         * @param PEType     Processing element type (i.e the type of the cluster it belongs. ex: ARM_A7).
         * @param expression Expression of the timing to set.
         */
        void
        setExecutionTiming(const spider::pisdf::Vertex *vertex, uint32_t PEType, const std::string &expression);

    private:

        std::vector<std::vector<bool>> mappingConstraintsVector_;
        spider::vector<spider::vector<Expression>> executionTimingsVector_;

        /* === Private method(s) === */
    };
}

#endif //SPIDER2_SCENARIO_H
