/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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
#ifndef SPIDER2_PISDFTYPES_H
#define SPIDER2_PISDFTYPES_H

/* === Includes === */

#include <cstdint>

/* === Enumeration(s) === */

/**
 * @brief PiSDF parameter types
 */
enum class PiSDFParamType : std::uint8_t {
    STATIC,            /*! Static parameter: expression is evaluated at startup only once */
    HERITED,           /*! Herited parameter: if inheritance is fully static == STATIC type, else DYNAMIC_DEPENDENT */
    DYNAMIC,           /*! Dynamic parameter: value is set at runtime */
    DYNAMIC_DEPENDENT, /*! Dynamic parameter: expression is evaluated at runtime */
};

/**
 * @brief Type of PiSDF vertices
 */
enum class PiSDFVertexType : std::uint8_t {
    NORMAL,         /*! Normal actor subtype */
    DUPLICATE,      /*! Duplicate actor subtype */
    ROUNDBUFFER,    /*! Roundbuffer actor subtype */
    FORK,           /*! Fork actor subtype */
    JOIN,           /*! Join actor subtype */
    INIT,           /*! Init actor subtype */
    END,            /*! End actor subtype */
    CONFIG,         /*! Config vertex subtype */
    HIERARCHICAL,   /*! Hierarchical vertex subtype */
    INTERFACE,      /*! Interface vertex subtype */
    DELAY,          /*! Delay vertex subtype */
};

enum class PiSDFInterfaceType : std::uint8_t {
    INPUT,          /*! Input interface type */
    OUTPUT,         /*! Output interface type */
};

#endif //SPIDER2_PISDFTYPES_H
