/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019)
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
#ifndef SPIDER2_MONITOR_H
#define SPIDER2_MONITOR_H

/* === Include(s) === */

#include <string>
#include <map>

namespace spider {

    /* === Class definition === */

    class Monitor {
    public:
        Monitor() = default;

        virtual ~Monitor() = default;

        /* === Method(s) === */

        inline bool registerEvent(const std::string &name, uint32_t ix) {
            if (events_[ix].empty()) {
                events_[ix] = name;
                return true;
            }
            return false;
        }

        virtual void startSampling() = 0;

        virtual void endSampling() = 0;

        /* === Getter(s) === */

        inline const std::string &eventName(uint32_t ix) const;

        inline const std::map<uint32_t, std::string> &events() const;

        /* === Setter(s) === */

    private:
        std::map<uint32_t, std::string> events_;
    };

    /* === Inline method(s) === */

    const std::string &Monitor::eventName(uint32_t ix) const {
        return events_.at(ix);
    }

    const std::map<uint32_t, std::string> &Monitor::events() const {
        return events_;
    }
}

#endif //SPIDER2_MONITOR_H
