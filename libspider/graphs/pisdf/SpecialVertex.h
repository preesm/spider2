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
#ifndef SPIDER2_SPECIALVERTEX_H
#define SPIDER2_SPECIALVERTEX_H

/* === Include(s) === */

#include <graphs/pisdf/ExecVertex.h>

namespace spider {
    namespace pisdf {

        /* === Class definition === */

        template<VertexType type>
        class SpecialVertex final : public ExecVertex {
        public:

            SpecialVertex() = delete;

            explicit SpecialVertex(std::string, StackID = StackID::PISDF) = delete;

            SpecialVertex(std::string, /* = Name = */
                          uint32_t,    /* = Input/Output edge count (depend on VertexType) = */
                          StackID stack = StackID::PISDF) = delete;

            SpecialVertex(std::string, /* = Name = */
                          uint32_t,    /* = Input edge count = */
                          uint32_t,    /* = Output edge count = */
                          StackID stack = StackID::PISDF) = delete;

            SpecialVertex(const SpecialVertex &other, StackID stack = StackID::PISDF) : ExecVertex(other, stack) { };

            SpecialVertex(SpecialVertex &&other) noexcept : ExecVertex(std::move(other)) { };

            ~SpecialVertex() override = default;

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override {
                visitor->visit(this);
            }

            /* === Getter(s) === */

            inline VertexType subtype() const override {
                return type;
            }

            /* === Getter(s) === */

            inline void setRepetitionValue(std::uint32_t value) override {
                ExecVertex::setRepetitionValue(value);
            }

        private:

        };

        /* === Define SpecialVertex types === */

        using ConfigVertex = SpecialVertex<VertexType::CONFIG>;

        using DelayVertex = SpecialVertex<VertexType::DELAY>;

        using ForkVertex = SpecialVertex<VertexType::FORK>;

        using JoinVertex = SpecialVertex<VertexType::JOIN>;

        using HeadVertex = SpecialVertex<VertexType::HEAD>;

        using TailVertex = SpecialVertex<VertexType::TAIL>;

        using RepeatVertex = SpecialVertex<VertexType::REPEAT>;

        using DuplicateVertex = SpecialVertex<VertexType::DUPLICATE>;

        using InitVertex = SpecialVertex<VertexType::INIT>;

        using EndVertex = SpecialVertex<VertexType::END>;


        /* === Define SpecialVertex specific override === */

        template<>
        inline void spider::pisdf::ConfigVertex::setRepetitionValue(uint32_t value) {
            if (value > 1) {
                throwSpiderException("Configure actor [%s] has repetition vector value of %"
                                             PRIu32
                                             " instead of 1.", name().c_str(), value);
            }
            repetitionValue_ = value;
        }

        template<>
        inline void spider::pisdf::DelayVertex::setRepetitionValue(uint32_t value) {
            if (value > 1) {
                throwSpiderException("Delay actor [%s] has repetition vector value of %"
                                             PRIu32
                                             " instead of 1.", name().c_str(), value);
            }
            repetitionValue_ = value;
        }

        /* === Define SpecialVertex constructor(s) === */

        // We need to disable auto formatter here because it does really weird stuff for this particular layout
        // @formatter:off

        /* === ConfigVertex === */

        template<>
        inline spider::pisdf::ConfigVertex::SpecialVertex(std::string name, uint32_t edgeINCount, uint32_t edgeOUTCount, StackID stack) :
                                            ExecVertex(std::move(name), edgeINCount, edgeOUTCount, stack) { }

        template<>
        inline spider::pisdf::ConfigVertex::SpecialVertex() :
                ConfigVertex("unnamed-cfg", 0, 0, StackID::PISDF) { }

        /* === DelayVertex === */

        template<>
        inline spider::pisdf::DelayVertex::SpecialVertex(std::string name, StackID stack) :
                                           ExecVertex(std::move(name),1, 1, stack) { }

        template<>
        inline spider::pisdf::DelayVertex::SpecialVertex() :
                DelayVertex("unnamed-delay", StackID::PISDF) { }

        /* === ForkVertex === */

        template<>
        inline spider::pisdf::ForkVertex::SpecialVertex(std::string name, uint32_t edgeOUTCount, StackID stack) :
                                          ExecVertex(std::move(name), 1, edgeOUTCount, stack) { }

        template<>
        inline spider::pisdf::ForkVertex::SpecialVertex() :
                ForkVertex("unnamed-fork", 0, StackID::PISDF) { }

        /* === JoinVertex === */

        template<>
        inline spider::pisdf::JoinVertex::SpecialVertex(std::string name, uint32_t edgeINCount, StackID stack) :
                                          ExecVertex(std::move(name), edgeINCount, 1, stack) { }

        template<>
        inline spider::pisdf::JoinVertex::SpecialVertex() :
                JoinVertex("unnamed-join", 0, StackID::PISDF) { }

        /* === HeadVertex === */

        template<>
        inline spider::pisdf::HeadVertex::SpecialVertex(std::string name, uint32_t edgeINCount, StackID stack) :
                                          ExecVertex(std::move(name), edgeINCount, 1, stack) { }

        template<>
        inline spider::pisdf::HeadVertex::SpecialVertex() :
                HeadVertex("unnamed-head", 0, StackID::PISDF) { }

        /* === TailVertex === */

        template<>
        inline spider::pisdf::TailVertex::SpecialVertex(std::string name, uint32_t edgeINCount, StackID stack) :
                                          ExecVertex(std::move(name), edgeINCount, 1, stack) { }

        template<>
        inline spider::pisdf::TailVertex::SpecialVertex() :
                TailVertex("unnamed-tail", 0, StackID::PISDF) { }

        /* === RepeatVertex === */

        template<>
        inline spider::pisdf::RepeatVertex::SpecialVertex(std::string name, StackID stack) :
                                            ExecVertex(std::move(name),1, 1, stack) { }

        template<>
        inline spider::pisdf::RepeatVertex::SpecialVertex() :
                RepeatVertex("unnamed-repeat", StackID::PISDF) { }

        /* === DuplicateVertex === */

        template<>
        inline spider::pisdf::DuplicateVertex::SpecialVertex(std::string name, uint32_t edgeOUTCount, StackID stack) :
                                               ExecVertex(std::move(name),1, edgeOUTCount, stack) { }

        template<>
        inline spider::pisdf::DuplicateVertex::SpecialVertex() :
                DuplicateVertex("unnamed-duplicate", 0, StackID::PISDF) { }

        /* === InitVertex === */

        template<>
        inline spider::pisdf::InitVertex::SpecialVertex(std::string name, StackID stack) :
                                          ExecVertex(std::move(name),0, 1, stack) { }

        template<>
        inline spider::pisdf::InitVertex::SpecialVertex() :
                InitVertex("unnamed-init", StackID::PISDF) { }

        /* === EndVertex === */

        template<>
        inline spider::pisdf::EndVertex::SpecialVertex(std::string name, StackID stack) :
                                         ExecVertex(std::move(name), 1, 0, stack) { }

        template<>
        inline spider::pisdf::EndVertex::SpecialVertex() :
                EndVertex("unnamed-end", StackID::PISDF) { }
    }
    // @formatter:on
}

#endif //SPIDER2_SPECIALVERTEX_H
