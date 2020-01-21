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

            explicit SpecialVertex(std::string) = delete;

            SpecialVertex(std::string, /* = Name = */
                          size_t       /* = Input/Output edge count (depend on VertexType) = */) = delete;

            SpecialVertex(std::string, /* = Name = */
                          size_t,      /* = Input edge count = */
                          size_t       /* = Output edge count = */) = delete;

            SpecialVertex(const SpecialVertex &) = default;

            SpecialVertex(SpecialVertex &&) noexcept = default;

            ~SpecialVertex() override = default;

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override {
                visitor->visit(this);
            }

            /* === Getter(s) === */

            inline VertexType subtype() const override {
                return type;
            }

            /* === Setter(s) === */

            inline void setRepetitionValue(uint32_t value) override {
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
        inline void ConfigVertex::setRepetitionValue(uint32_t value) {
            if (value > 1) {
                throwSpiderException("Configure actor [%s] has repetition vector value of %"
                                             PRIu32
                                             " instead of 1.", name().c_str(), value);
            }
            repetitionValue_ = value;
        }

        template<>
        inline void DelayVertex::setRepetitionValue(uint32_t value) {
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
        inline ConfigVertex::SpecialVertex(std::string name, size_t edgeINCount, size_t edgeOUTCount) :
                                            ExecVertex(std::move(name), edgeINCount, edgeOUTCount) { }

        template<>
        inline ConfigVertex::SpecialVertex() :
                ConfigVertex("unnamed-cfg", 0, 0) { }

        /* === DelayVertex === */

        template<>
        inline DelayVertex::SpecialVertex(std::string name) :
                                           ExecVertex(std::move(name),1, 1) { }

        template<>
        inline DelayVertex::SpecialVertex() :
                DelayVertex("unnamed-delay") { }

        /* === ForkVertex === */

        template<>
        inline ForkVertex::SpecialVertex(std::string name, size_t edgeOUTCount) :
                                          ExecVertex(std::move(name), 1, edgeOUTCount) { }

        template<>
        inline ForkVertex::SpecialVertex() :
                ForkVertex("unnamed-fork", 0) { }

        /* === JoinVertex === */

        template<>
        inline JoinVertex::SpecialVertex(std::string name, size_t edgeINCount) :
                                          ExecVertex(std::move(name), edgeINCount, 1) { }

        template<>
        inline JoinVertex::SpecialVertex() :
                JoinVertex("unnamed-join", 0) { }

        /* === HeadVertex === */

        template<>
        inline HeadVertex::SpecialVertex(std::string name, size_t edgeINCount) :
                                          ExecVertex(std::move(name), edgeINCount, 1) { }

        template<>
        inline HeadVertex::SpecialVertex() :
                HeadVertex("unnamed-head", 0) { }

        /* === TailVertex === */

        template<>
        inline TailVertex::SpecialVertex(std::string name, size_t edgeINCount) :
                                          ExecVertex(std::move(name), edgeINCount, 1) { }

        template<>
        inline TailVertex::SpecialVertex() :
                TailVertex("unnamed-tail", 0) { }

        /* === RepeatVertex === */

        template<>
        inline RepeatVertex::SpecialVertex(std::string name) :
                                            ExecVertex(std::move(name),1, 1) { }

        template<>
        inline RepeatVertex::SpecialVertex() :
                RepeatVertex("unnamed-repeat") { }

        /* === DuplicateVertex === */

        template<>
        inline DuplicateVertex::SpecialVertex(std::string name, size_t edgeOUTCount) :
                                               ExecVertex(std::move(name),1, edgeOUTCount) { }

        template<>
        inline DuplicateVertex::SpecialVertex() :
                DuplicateVertex("unnamed-duplicate", 0) { }

        /* === InitVertex === */

        template<>
        inline InitVertex::SpecialVertex(std::string name) :
                                          ExecVertex(std::move(name),0, 1) { }

        template<>
        inline InitVertex::SpecialVertex() :
                InitVertex("unnamed-init") { }

        /* === EndVertex === */

        template<>
        inline EndVertex::SpecialVertex(std::string name) :
                                         ExecVertex(std::move(name), 1, 0) { }

        template<>
        inline EndVertex::SpecialVertex() :
                EndVertex("unnamed-end") { }
    }
    // @formatter:on
}

#endif //SPIDER2_SPECIALVERTEX_H
