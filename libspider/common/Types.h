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
#ifndef SPIDER2_TYPES_H
#define SPIDER2_TYPES_H

/* === Include(s) === */

#include <cstdint>
#include <cinttypes>

/* === Defining short aliases for all types === */

/* == Signed integer == */
using i8 = int_least8_t;
using i16 = int_least16_t;
using i32 = int_least32_t;
using i64 = int_least64_t;
using ifast8 = int_fast8_t;
using ifast16 = int_fast16_t;
using ifast32 = int_fast32_t;
using ifast64 = int_fast64_t;

/* == Unsigned integer == */
using u8 = uint_least8_t;
using u16 = uint_least16_t;
using u32 = uint_least32_t;
using u64 = uint_least64_t;
using ufast8 = uint_fast8_t;
using ufast16 = uint_fast16_t;
using ufast32 = uint_fast32_t;
using ufast64 = uint_fast64_t;

/* == Macros for printf format == */

/* == Signed integer == */
#define PRINT_I8        PRIdLEAST8
#define PRINT_I16       PRIdLEAST16
#define PRINT_I32       PRIdLEAST32
#define PRINT_I64       PRIdLEAST64
#define PRINT_IFAST8    PRIdFAST8
#define PRINT_IFAST16   PRIdFAST16
#define PRINT_IFAST32   PRIdFAST32
#define PRINT_IFAST64   PRIdFAST64

/* == Unsigned integer == */
#define PRINT_U8        PRIuLEAST8
#define PRINT_U16       PRIuLEAST16
#define PRINT_U32       PRIuLEAST32
#define PRINT_U64       PRIuLEAST64
#define PRINT_UFAST8    PRIuFAST8
#define PRINT_UFAST16   PRIuFAST16
#define PRINT_UFAST32   PRIuFAST32
#define PRINT_UFAST64   PRIuFAST64

#endif //SPIDER2_TYPES_H
