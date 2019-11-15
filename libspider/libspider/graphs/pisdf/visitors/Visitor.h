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
#ifndef SPIDER2_VISITOR_H
#define SPIDER2_VISITOR_H

/* === Include(s) === */

#include <graphs/pisdf/common/Types.h>
#include <common/Exception.h>

namespace Spider {
    namespace PiSDF {

        /* === Class definition === */

        class Visitor {
        public:
            Visitor() = default;

            ~Visitor() = default;

            /* === Method(s) === */

            virtual void visit(Graph *) = 0;

            virtual void visit(ExecVertex *) = 0;

            virtual void visit(ConfigVertex *) = 0;

            virtual void visit(ForkVertex *) = 0;

            virtual void visit(JoinVertex *) = 0;

            virtual void visit(HeadVertex *) = 0;

            virtual void visit(TailVertex *) = 0;

            virtual void visit(DuplicateVertex *) = 0;

            virtual void visit(RepeatVertex *) = 0;

            virtual void visit(InitVertex *) = 0;

            virtual void visit(EndVertex *) = 0;

            virtual void visit(Interface *) = 0;

            virtual void visit(InputInterface *) = 0;

            virtual void visit(OutputInterface *) = 0;

            virtual void visit(Param *) = 0;

            virtual void visit(DynamicParam *) = 0;

            virtual void visit(InHeritedParam *) = 0;

        private:

        };

        class DefaultVisitor : public Visitor {
        public:
            /* === Method(s) === */

            inline void visit(Graph *) override {
                throwSpiderException("unsupported visitor type: Graph.");
            }

            void visit(ExecVertex *) override = 0;

            void visit(ConfigVertex *vertex) override;

            void visit(ForkVertex *vertex) override;

            void visit(JoinVertex *vertex) override;

            void visit(HeadVertex *vertex) override;

            void visit(TailVertex *vertex) override;

            void visit(DuplicateVertex *vertex) override;

            void visit(RepeatVertex *vertex) override;

            void visit(InitVertex *vertex) override;

            void visit(EndVertex *vertex) override;

            inline void visit(Interface *) override {
                throwSpiderException("unsupported visitor type: Interface.");
            }

            inline void visit(InputInterface *) override {
                throwSpiderException("unsupported visitor type: InputInterface.");
            }

            inline void visit(OutputInterface *) override {
                throwSpiderException("unsupported visitor type: OutputInterface.");
            }

            inline void visit(Param *) override {
                throwSpiderException("unsupported visitor type: Param.");
            }

            inline void visit(DynamicParam *) override {
                throwSpiderException("unsupported visitor type: DynamicParam.");
            }

            inline void visit(InHeritedParam *) override {
                throwSpiderException("unsupported visitor type: InHeritedParam.");
            }
        };

    }
}
#endif //SPIDER2_VISITOR_H
