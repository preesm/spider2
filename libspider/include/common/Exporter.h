/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
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
#ifndef SPIDER2_EXPORTER_H
#define SPIDER2_EXPORTER_H

/* === Include(s) === */

#include <string>

namespace spider {
    /* === Class definition === */

    struct Exporter {
        Exporter() = default;

        virtual ~Exporter() = default;

        /* === Method(s) === */

        /**
         * @brief Print the exported product to a default file path.
         */
        virtual void print() const = 0;

        /**
         * @brief Open file of path "path" and print the product to the file.
         * @param path Path of the resulting file.
         */
        void printFromPath(const std::string &path) const;

        /**
         * @brief Print the product to a given opened file.
         * @param file Pointer to the file to which the product will be printed.
         */
        virtual void printFromFile(FILE *file) const = 0;

    };
}

#endif //SPIDER2_EXPORTER_H
