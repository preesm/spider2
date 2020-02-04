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

        /* === Class definition(s) === */

        /* === ConfigVertex === */

        class ConfigVertex final : public ExecVertex {
        public:

            explicit ConfigVertex(std::string name = "unnamed-config",
                                  size_t edgeINCount = 0,
                                  size_t edgeOUTCount = 0);

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override { visitor->visit(this); };

            Vertex *emptyClone(std::string name) override;

            /* === Getter(s) === */

            inline VertexType subtype() const override { return VertexType::CONFIG; };

            /* === Setter(s) === */

            void setRepetitionValue(uint32_t value) override;
        };

        /* === DelayVertex === */

        class DelayVertex final : public ExecVertex {
        public:

            explicit DelayVertex(std::string name = "unnamed-delay");

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override { visitor->visit(this); };

            Vertex *emptyClone(std::string name) override;

            /* === Getter(s) === */

            inline VertexType subtype() const override { return VertexType::DELAY; };

            /* === Setter(s) === */

            void setRepetitionValue(uint32_t value) override;
        };

        /* === ForkVertex === */

        class ForkVertex final : public ExecVertex {
        public:

            explicit ForkVertex(std::string name = "unnamed-fork", size_t edgeOUTCount = 0);

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override { visitor->visit(this); };

            Vertex *emptyClone(std::string name) override;

            /* === Getter(s) === */

            inline VertexType subtype() const override { return VertexType::FORK; };
        };

        /* === JoinVertex === */

        class JoinVertex final : public ExecVertex {
        public:

            explicit JoinVertex(std::string name = "unnamed-join", size_t edgeINCount = 0);

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override { visitor->visit(this); };

            Vertex *emptyClone(std::string name) override;

            /* === Getter(s) === */

            inline VertexType subtype() const override { return VertexType::JOIN; };
        };

        /* === HeadVertex === */

        class HeadVertex final : public ExecVertex {
        public:

            explicit HeadVertex(std::string name = "unnamed-head", size_t edgeINCount = 0);

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override { visitor->visit(this); };

            Vertex *emptyClone(std::string name) override;

            /* === Getter(s) === */

            inline VertexType subtype() const override { return VertexType::HEAD; };
        };

        /* === TailVertex === */

        class TailVertex final : public ExecVertex {
        public:

            explicit TailVertex(std::string name = "unnamed-tail", size_t edgeINCount = 0);

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override { visitor->visit(this); };

            Vertex *emptyClone(std::string name) override;

            /* === Getter(s) === */

            inline VertexType subtype() const override { return VertexType::TAIL; };
        };

        /* === RepeatVertex === */

        class RepeatVertex final : public ExecVertex {
        public:

            explicit RepeatVertex(std::string name = "unnamed-repeat");

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override { visitor->visit(this); };

            Vertex *emptyClone(std::string name) override;

            /* === Getter(s) === */

            inline VertexType subtype() const override { return VertexType::REPEAT; };
        };

        /* === DuplicateVertex === */

        class DuplicateVertex final : public ExecVertex {
        public:

            explicit DuplicateVertex(std::string name = "unnamed-duplicate", size_t edgeOUTCount = 0);

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override { visitor->visit(this); };

            Vertex *emptyClone(std::string name) override;

            /* === Getter(s) === */

            inline VertexType subtype() const override { return VertexType::DUPLICATE; };
        };

        /* === InitVertex === */

        class InitVertex final : public ExecVertex {
        public:

            explicit InitVertex(std::string name = "unnamed-init");

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override { visitor->visit(this); };

            Vertex *emptyClone(std::string name) override;

            /* === Getter(s) === */

            inline VertexType subtype() const override { return VertexType::INIT; };
        };

        /* === EndVertex === */

        class EndVertex final : public ExecVertex {
        public:

            explicit EndVertex(std::string name = "unnamed-end");

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override { visitor->visit(this); };

            Vertex *emptyClone(std::string name) override;

            /* === Getter(s) === */

            inline VertexType subtype() const override { return VertexType::END; };
        };

    }
}

#endif //SPIDER2_SPECIALVERTEX_H
