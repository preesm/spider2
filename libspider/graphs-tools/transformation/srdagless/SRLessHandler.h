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
#ifndef SPIDER2_SRLESSHANDLER_H
#define SPIDER2_SRLESSHANDLER_H

/* === Include(s) === */

#include <common/Types.h>
#include <containers/vector.h>
#include <containers/unordered_map.h>

namespace spider {

    /* === Forward declaration(s) === */

    namespace pisdf {

        class Graph;

        class Param;

        class Vertex;

    }

    namespace srdagless {

        /* === Class definition === */

        class SRLessHandler {
        public:
            explicit SRLessHandler();

            ~SRLessHandler() = default;

            /* === Struct(s) === */

            struct Dependency {
                pisdf::Vertex *vertex_;
                i32 first_;
                i32 last_;
            };

            /* === Method(s) === */

            /**
             * @brief Compute repetition value for a given firing of a graph.
             * @param graph       Pointer to the graph.
             * @param graphFiring Firing of the graph.
             */
            void computeRV(pisdf::Graph *graph, u32 graphFiring);

            /**
             * @brief Compute the lower consumption dependency of a given edge with respect to a given firing
             *        of a vertex inside a given firing of a graph (ex: firing 2 of actor A of firing 3 of graph G).
             * @param edge          Pointer to the edge.
             * @param vertexFiring  Firing of the vertex.
             * @param graph         Pointer to the graph (necessary to retrieve proper parameters).
             * @param graphFiring   Firing of the graph.
             * @return value of the dependency.
             */
            ifast64
            computeConsLowerDep(pisdf::Edge *edge, u32 vertexFiring, pisdf::Graph *graph, u32 graphFiring) const;

            /**
             * @brief Compute the upper consumption dependency of a given edge with respect to a given firing
             *        of a vertex inside a given firing of a graph (ex: firing 2 of actor A of firing 3 of graph G).
             * @param edge          Pointer to the edge.
             * @param vertexFiring  Firing of the vertex.
             * @param graph         Pointer to the graph (necessary to retrieve proper parameters).
             * @param graphFiring   Firing of the graph.
             * @return value of the dependency.
             */
            ifast64
            computeConsUpperDep(pisdf::Edge *edge, u32 vertexFiring, pisdf::Graph *graph, u32 graphFiring) const;

            /**
             * @brief Copy parameters for a given graph firing.
             * @param graph         Pointer to the graph.
             * @param graphRepCount Repetition value of the graph.
             * @param parentFiring  Firing of the parent graph.
             */
            void copyParameters(pisdf::Graph *graph, u32 graphRepCount, u32 parentFiring);

            void createsProductionDependencies(pisdf::Vertex *vertex, u32 graphFiring);

            void createsConsumptionDependencies(pisdf::Vertex *vertex, u32 graphFiring);

            void addVertexToBeScheduled(pisdf::Vertex *vertex);

            void clearVertexToBeScheduled();

            /* === Getter(s) === */

            const vector<std::shared_ptr<pisdf::Param>> &getParameters(pisdf::Graph *graph, u32 graphFiring) const;

            const vector<pisdf::Vertex*> &getVerticesToSchedule() const;

            /**
             * @brief Get the repetition value of a vertex for a given graph firing.
             * @param vertex       Pointer to the vertex.
             * @param graphFiring  Firing of the graph.
             * @return repetition value of the vertex.
             */
            u32 getRepetitionValue(pisdf::Vertex *vertex, u32 graphFiring) const;


            /* === Setter(s) === */

            /**
             * @brief Set the value of a parameter for a given firing of its corresponding graph.
             * @param param        Pointer to the parameter in the pisdf representation.
             * @param graphFiring  Global firing of the graph (taking into account the hierarchy).
             * @param value        Value to set.
             */
            void setParamValue(pisdf::Param *param, u32 graphFiring, i64 value);

        private:
            using FiringVector = vector<vector<u32>>;
            using ParamVector = vector<vector<std::shared_ptr<pisdf::Param>>>;
            using DependencyVector = vector<vector<Dependency>>;

            unordered_map<pisdf::Graph *, vector<DependencyVector>> prodDependencies_;
            unordered_map<pisdf::Graph *, vector<DependencyVector>> consDependencies_;
            unordered_map<pisdf::Graph *, ParamVector> parameters_;
            unordered_map<pisdf::Graph *, FiringVector> graph2RV_;
            vector<pisdf::Vertex*> verticesToSchedule_;

            /* === Private methods === */

            inline static ParamVector makeParamVector() {
                return factory::vector<vector<std::shared_ptr<pisdf::Param>>>(StackID::TRANSFO);
            }

            inline static DependencyVector makeDependencyVector() {
                return factory::vector<vector<Dependency>>(StackID::TRANSFO);
            }

            inline static FiringVector makeRVVector() {
                return factory::vector<vector<u32>>(StackID::TRANSFO);
            }

        };
    }
}
#endif //SPIDER2_SRLESSHANDLER_H
