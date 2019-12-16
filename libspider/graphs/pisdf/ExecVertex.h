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
#ifndef SPIDER2_EXECVERTEX_H
#define SPIDER2_EXECVERTEX_H

/* === Include(s) === */

#include <cstdint>
#include <string>
#include <graphs/pisdf/Vertex.h>

namespace spider {

    namespace pisdf {

        /* === Class definition === */

        class ExecVertex : public Vertex {
        public:
            explicit ExecVertex(std::string name = "unnamed-execvertex",
                                uint32_t edgeINCount = 0,
                                uint32_t edgeOUTCount = 0,
                                StackID stack = StackID::PISDF) : Vertex(std::move(name),
                                                                         edgeINCount,
                                                                         edgeOUTCount,
                                                                         stack) { }

            ExecVertex(const ExecVertex &other, StackID stack = StackID::PISDF) : Vertex(other, stack) {
                refinementIx_ = other.refinementIx_;
                jobIx_ = other.jobIx_;
            }

            ExecVertex(ExecVertex &&other) noexcept : Vertex(std::move(other)) {
                std::swap(refinementIx_, other.refinementIx_);
                std::swap(jobIx_, other.jobIx_);
            }

            friend CloneVisitor;

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override {
                visitor->visit(this);
            }

            /* === Getter(s) === */

            inline VertexType subtype() const override {
                return VertexType::NORMAL;
            }

            inline bool executable() const override {
                return true;
            }

            /**
             * @brief Get the refinement index in the global register associated to this vertex.
             * @return index of the refinement, UINT32_MAX if not set.
             */
            inline uint32_t refinementIx() const {
                return refinementIx_;
            }

            /**
             * @brief Get the job ix associated to this vertex.
             * @remark In the case of @refitem VertexType::CONFIG, the value match the one of the corresponding dynamic job.
             * @return ix of the job, UINT32_MAX if not set.
             */
            inline size_t jobIx() const {
                return jobIx_;
            }

            /**
             * @brief Ensure that no vertex inheriting from ExecVertex will override this method.
             * @return false.
             */
            inline bool hierarchical() const final {
                return false;
            }

            /* === Setter(s) === */

            /**
             * @brief Set the refinement index of the vertex.
             * @param ix  Index to set.
             */
            inline void setRefinementIx(uint32_t ix) {
                refinementIx_ = ix;
            }

            /**
             * @brief Set the job ix of the vertex.
             * @param ix  Ix to set.
             */
            inline void setJobIx(size_t ix) {
                jobIx_ = ix;
            }

        protected:
            size_t jobIx_ = UINT32_MAX;
            uint32_t refinementIx_ = UINT32_MAX;

        };
    }
}
#endif //SPIDER2_EXECVERTEX_H
