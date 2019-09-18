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

#include <unordered_map>
#include <cstdint>
#include <vector>
#include <graphs-tools/expression-parser/Expression.h>

/* === Forward declaration(s) === */

class PiSDFGraph;

class PiSDFVertex;

class ProcessingElement;

namespace Spider {

    /* === Class definition === */

    class Scenario {
    public:

        explicit Scenario(const PiSDFGraph *graph);

        ~Scenario() = default;

        /* === Method(s) === */

        /* === Getter(s) === */

        /**
         * @brief Get the mappable constraints of a vertex for all PE in the platform.
         * @param vertex  Vertex to evaluate.
         * @return mapping constraints vector.
         * @throws @refitem std::out_of_range if vertex does not exist in the lookup table.
         */
        inline const std::vector<bool> &mappingConstraints(const PiSDFVertex *vertex) const;

        /**
         * @brief Get the execution timings of a given vertex for all PE type in the platform.
         * @param vertex   Vertex to evaluate.
         * @return execution timings vector.
         * @throws @refitem std::out_of_range if vertex does not exist in the lookup table.
         */
        inline const Spider::vector<Expression> &executionTimings(const PiSDFVertex *vertex) const;

        /**
         * @brief Get the mappable constraint of a vertex on a given PE.
         * @param vertex  Vertex to evaluate.
         * @param PE      PE to evaluate.
         * @return true if vertex is mappable on PE, false else.
         * @throws @refitem std::out_of_range if vertex or PE do not exist in the lookup table.
         */
        bool mappingConstraint(const PiSDFVertex *vertex, const ProcessingElement *PE) const;

        /**
         * @brief Get the execution timing of a given vertex on a specific PE type.
         * @param vertex   Vertex to evaluate.
         * @param PEType   PE type to evaluate.
         * @return execution timing.
         * @throws @refitem std::out_of_range if vertex or PEType do not exist in the lookup table.
         */
        inline std::int64_t executionTiming(const PiSDFVertex *vertex, std::uint32_t PEType) const;

        /**
         * @brief Get the execution timing of a given vertex on a specific PE.
         * @remark Timing are not PE specific, i.e all PE of a same PE type share the same timing.
         * @param vertex   Vertex to evaluate.
         * @param PEType   PE to evaluate.
         * @return execution timing.
         * @throws @refitem std::out_of_range if vertex or PEType do not exist in the lookup table.
         */
        std::int64_t executionTiming(const PiSDFVertex *vertex, const ProcessingElement *PE) const;

        /* === Setter(s) === */

        /**
         * @brief Set the mapping constraints of a given vertex for all possible processing elements of the platform.
         * @param vertex        Vertex for which constraints are set.
         * @param constraints   Constraints to set (as initialization list).
         */
        inline void setMappingConstraints(const PiSDFVertex *vertex, const std::initializer_list<bool> &constraints);

        /**
         * @brief Set the mapping constraints of a given vertex for all possible processing elements of the platform.
         * @param vertex    Vertex for which constraints are set.
         * @param PECount   Number of PE in the platform.
         * @param value     Constraint value to set (true if mappable, false else).
         */
        inline void setMappingConstraints(const PiSDFVertex *vertex, std::uint32_t PECount, bool value);

        /**
         * @brief Set the mapping constraint of a given vertex for a given processing element of the platform.
         * @param vertex       Vertex for which constraints are set.
         * @param spiderPEIx   Ix of the PE in spider.
         * @param value        Constraint to set (true if mappable, false else)
         */
        inline void setMappingConstraint(const PiSDFVertex *vertex, std::uint32_t spiderPEIx, bool value);

        /**
         * @brief Set the mapping constraint of a given vertex for a given processing element of the platform.
         * @param vertex  Vertex for which constraints are set.
         * @param PE      Processing element.
         * @param value   Constraint to set (true if mappable, false else)
         */
        void setMappingConstraint(const PiSDFVertex *vertex, const ProcessingElement *PE, bool value);

        /**
         * @brief Set the execution timings of a given vertex for all possible processing elements of the platform.
         * @param vertex    Vertex for which constraints are set.
         * @param timings   Timings to set (as initialization list).
         */
        inline void setExecutionTimings(const PiSDFVertex *vertex, const std::initializer_list<std::int64_t> &timings);

        /**
         * @brief Set the execution timings of a given vertex for all possible processing elements of the platform.
         * @param vertex    Vertex for which constraints are set.
         * @param PETypeCount   Number of PE in the platform.
         * @param value     Constraint value to set (true if mappable, false else).
         */
        inline void setExecutionTimings(const PiSDFVertex *vertex, std::uint32_t PETypeCount, std::int64_t value);

        /**
         * @brief Set the execution timing of a given vertex for a given processing element type of the platform.
         * @remark This method will replace current value of the timing for all PE sharing the same PEType.
         * @param vertex  Vertex for which timing is set.
         * @param PE      Processing element.
         * @param value   Timing to set.
         */
        void setExecutionTiming(const PiSDFVertex *vertex, const ProcessingElement *PE, std::int64_t value);

        /**
         * @brief Set the execution timing of a given vertex for a given processing element type of the platform.
         * @remark This method will replace current value of the timing for all PE sharing the same PEType.
         * @param vertex     Vertex for which timing is set.
         * @param PE         Processing element.
         * @param expression Expression of the timing to set.
         */
        void setExecutionTiming(const PiSDFVertex *vertex, const ProcessingElement *PE, const std::string &expression);

        /**
         * @brief Set the execution timing of a given vertex for a given processing element type of the platform.
         * @remark This method will replace current value of the timing.
         * @param vertex  Vertex for which timing is set.
         * @param PEType  Processing element type (i.e the type of the cluster it belongs. ex: ARM_A7).
         * @param value   Timing to set.
         */
        inline void setExecutionTiming(const PiSDFVertex *vertex, std::uint32_t PEType, std::int64_t value);

        /**
         * @brief Set the execution timing of a given vertex for a given processing element type of the platform.
         * @remark This method will replace current value of the timing.
         * @param vertex     Vertex for which timing is set.
         * @param PEType     Processing element type (i.e the type of the cluster it belongs. ex: ARM_A7).
         * @param expression Expression of the timing to set.
         */
        void setExecutionTiming(const PiSDFVertex *vertex, std::uint32_t PEType, const std::string &expression);

    private:

        std::unordered_map<const PiSDFVertex *, std::vector<bool>> vertexMappingConstraintsMap_;
        std::unordered_map<const PiSDFVertex *, Spider::vector<Expression>> vertexExecutionTimingsMap_;


        /* === Private method(s) === */
    };

    /* === Inline method(s) === */

    const std::vector<bool> &Scenario::mappingConstraints(const PiSDFVertex *vertex) const {
        return vertexMappingConstraintsMap_.at(vertex);
    }

    const Spider::vector<Expression> &Scenario::executionTimings(const PiSDFVertex *vertex) const {
        return vertexExecutionTimingsMap_.at(vertex);
    }

    std::int64_t Scenario::executionTiming(const PiSDFVertex *vertex, std::uint32_t PEType) const {
        auto &timings = vertexExecutionTimingsMap_.at(vertex);
        return timings.at(PEType).evaluate();
    }

    void Scenario::setMappingConstraints(const PiSDFVertex *vertex, const std::initializer_list<bool> &constraints) {
        if (vertexMappingConstraintsMap_.size() != constraints.size()) {
            throwSpiderException("Initialization list of constraints should be of size: %u",
                                 vertexMappingConstraintsMap_.size());
        }
        vertexMappingConstraintsMap_[vertex] = constraints;
    }

    void Scenario::setMappingConstraints(const PiSDFVertex *vertex, std::uint32_t PECount, bool value) {
        vertexMappingConstraintsMap_[vertex] = std::vector<bool>(PECount, value);
    }

    void Scenario::setMappingConstraint(const PiSDFVertex *vertex, std::uint32_t spiderPEIx, bool value) {
        auto &contraints = vertexMappingConstraintsMap_.at(vertex);
        contraints.at(spiderPEIx) = value;
    }

    void Scenario::setExecutionTimings(const PiSDFVertex *vertex, const std::initializer_list<std::int64_t> &timings) {
        if (vertexExecutionTimingsMap_.size() != timings.size()) {
            throwSpiderException("Initialization list of timings should be of size: %u",
                                 vertexExecutionTimingsMap_.size());
        }
        vertexExecutionTimingsMap_[vertex].clear();
        for (const auto &timing : timings) {
            vertexExecutionTimingsMap_[vertex].push_back(Expression(timing));
        }
    }

    void Scenario::setExecutionTimings(const PiSDFVertex *vertex, std::uint32_t PETypeCount, std::int64_t value) {
        vertexExecutionTimingsMap_[vertex] = Spider::vector<Expression>(PETypeCount, Expression(value));
    }

    void Scenario::setExecutionTiming(const PiSDFVertex *vertex, std::uint32_t PEType, std::int64_t value) {
        auto &timings = vertexExecutionTimingsMap_.at(vertex);
        auto &timing = timings.at(PEType);
        timing = Expression(value);
    }
}

#endif //SPIDER2_SCENARIO_H
