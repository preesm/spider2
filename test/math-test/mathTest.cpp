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

#include <gtest/gtest.h>
#include <common/Math.h>


class mathTest : public ::testing::Test {
protected:
};

TEST_F(mathTest, test) {
    uint32_t a = UINT32_MAX;
    ASSERT_EQ(spider::math::saturateAdd(a, 1), UINT32_MAX) << "math::saturateAdd(uint32_t) failed";
    ASSERT_EQ(spider::math::saturateAdd(a - 10, 10), UINT32_MAX) << "math::saturateAdd(uint32_t) failed";
    ASSERT_EQ(spider::math::saturateAdd(UINT64_MAX, 1), UINT64_MAX) << "math::saturateAdd(uint64_t) failed";
    ASSERT_EQ(spider::math::saturateAdd(UINT64_MAX - 10, 10), UINT64_MAX) << "math::saturateAdd(uint64_t) failed";
    ASSERT_EQ(spider::math::saturateAdd(31415926535u, 10), 31415926545u) << "math::saturateAdd(uint64_t) failed";


    ASSERT_EQ(spider::math::abs(static_cast<int16_t>(-2)), 2) << "math::abs(int16_t) failed";
    ASSERT_EQ(spider::math::abs(static_cast<int32_t>(-2)), 2) << "math::abs(int32_t) failed";
    ASSERT_EQ(spider::math::abs(static_cast<int64_t>(-2)), 2) << "math::abs(int64_t) failed";
    ASSERT_EQ(spider::math::abs(static_cast<int16_t>(2)), 2) << "math::abs(int16_t) failed";
    ASSERT_EQ(spider::math::abs(static_cast<int32_t>(2)), 2) << "math::abs(int32_t) failed";
    ASSERT_EQ(spider::math::abs(static_cast<int64_t>(2)), 2) << "math::abs(int64_t) failed";
}