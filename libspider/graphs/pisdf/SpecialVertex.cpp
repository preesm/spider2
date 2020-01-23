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

/* === Include(s) === */

#include <graphs/pisdf/SpecialVertex.h>

/* === Function(s) definition === */

/* === ConfigVertex === */

spider::pisdf::ConfigVertex::ConfigVertex(std::string name, size_t edgeINCount, size_t edgeOUTCount) :
        ExecVertex(std::move(name), edgeINCount, edgeOUTCount) {

}

void spider::pisdf::ConfigVertex::setRepetitionValue(uint32_t value) {
    if (value > 1) {
        throwSpiderException("Configure actor [%s] has repetition vector value of %"
                                     PRIu32
                                     " instead of 1.", name().c_str(), value);
    }
    repetitionValue_ = value;
}

spider::pisdf::ConfigVertex::ConfigVertex(std::string name,
                                          size_t edgeINCount,
                                          size_t edgeOUTCount,
                                          size_t paramINCount,
                                          size_t paramOUTCount,
                                          const spider::pisdf::ConfigVertex *reference) :
        ExecVertex(std::move(name), edgeINCount, edgeOUTCount, paramINCount, paramOUTCount, reference) {

}

/* === DelayVertex === */

spider::pisdf::DelayVertex::DelayVertex(std::string name) : ExecVertex(std::move(name), 1, 1) {

}

spider::pisdf::DelayVertex::DelayVertex(std::string name,
                                        size_t,
                                        size_t,
                                        size_t,
                                        size_t,
                                        const spider::pisdf::DelayVertex *reference) :
        ExecVertex(std::move(name), 1, 1, 0, 0, reference) {

}

void spider::pisdf::DelayVertex::setRepetitionValue(uint32_t value) {
    if (value > 1) {
        throwSpiderException("Delay actor [%s] has repetition vector value of %"
                                     PRIu32
                                     " instead of 1.", name().c_str(), value);
    }
    repetitionValue_ = value;
}

/* === ForkVertex === */

spider::pisdf::ForkVertex::ForkVertex(std::string name, size_t edgeOUTCount) : ExecVertex(std::move(name),
                                                                                          1,
                                                                                          edgeOUTCount) {

}

spider::pisdf::ForkVertex::ForkVertex(std::string name,
                                      size_t,
                                      size_t edgeOUTCount,
                                      size_t,
                                      size_t,
                                      const spider::pisdf::ForkVertex *reference) :
        ExecVertex(std::move(name), 1, edgeOUTCount, 0, 0, reference) {

}

/* === JoinVertex === */

spider::pisdf::JoinVertex::JoinVertex(std::string name, size_t edgeINCount) : ExecVertex(std::move(name),
                                                                                         edgeINCount,
                                                                                         1) {

}

spider::pisdf::JoinVertex::JoinVertex(std::string name,
                                      size_t edgeINCount,
                                      size_t,
                                      size_t,
                                      size_t,
                                      const spider::pisdf::JoinVertex *reference) :
        ExecVertex(std::move(name), edgeINCount, 1, 0, 0, reference) {

}

/* === HeadVertex === */

spider::pisdf::HeadVertex::HeadVertex(std::string name, size_t edgeINCount) : ExecVertex(std::move(name),
                                                                                         edgeINCount,
                                                                                         1) {

}

spider::pisdf::HeadVertex::HeadVertex(std::string name,
                                      size_t edgeINCount,
                                      size_t,
                                      size_t,
                                      size_t,
                                      const spider::pisdf::HeadVertex *reference) :
        ExecVertex(std::move(name), edgeINCount, 1, 0, 0, reference) {

}

/* === TailVertex === */

spider::pisdf::TailVertex::TailVertex(std::string name, size_t edgeINCount) : ExecVertex(std::move(name),
                                                                                         edgeINCount,
                                                                                         1) {

}

spider::pisdf::TailVertex::TailVertex(std::string name,
                                      size_t edgeINCount,
                                      size_t,
                                      size_t,
                                      size_t,
                                      const spider::pisdf::TailVertex *reference) :
        ExecVertex(std::move(name), edgeINCount, 1, 0, 0, reference) {

}

/* === RepeatVertex === */

spider::pisdf::RepeatVertex::RepeatVertex(std::string name) : ExecVertex(std::move(name), 1, 1) {

}

spider::pisdf::RepeatVertex::RepeatVertex(std::string name,
                                          size_t,
                                          size_t,
                                          size_t,
                                          size_t,
                                          const spider::pisdf::RepeatVertex *reference) :
        ExecVertex(std::move(name), 1, 1, 0, 0, reference) {

}

/* === DuplicateVertex === */

spider::pisdf::DuplicateVertex::DuplicateVertex(std::string name, size_t edgeOUTCount) : ExecVertex(std::move(name),
                                                                                                    1,
                                                                                                    edgeOUTCount) {

}

spider::pisdf::DuplicateVertex::DuplicateVertex(std::string name,
                                                size_t,
                                                size_t edgeOUTCount,
                                                size_t,
                                                size_t,
                                                const spider::pisdf::DuplicateVertex *reference) :
        ExecVertex(std::move(name), 1, edgeOUTCount, 0, 0, reference) {

}

/* === InitVertex === */

spider::pisdf::InitVertex::InitVertex(std::string name) : ExecVertex(std::move(name), 0, 1) {

}

spider::pisdf::InitVertex::InitVertex(std::string name,
                                      size_t,
                                      size_t,
                                      size_t,
                                      size_t,
                                      const spider::pisdf::InitVertex *reference) :
        ExecVertex(std::move(name), 0, 1, 0, 0, reference) {

}

/* === EndVertex === */

spider::pisdf::EndVertex::EndVertex(std::string name) : ExecVertex(std::move(name), 1, 0) {

}

spider::pisdf::EndVertex::EndVertex(std::string name,
                                    size_t,
                                    size_t,
                                    size_t,
                                    size_t,
                                    const spider::pisdf::EndVertex *reference) :
        ExecVertex(std::move(name), 1, 0, 0, 0, reference) {

}
