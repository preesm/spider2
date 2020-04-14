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
#include <common/Rational.h>
#include <common/Exception.h>


class mathTest : public ::testing::Test {
protected:
};

TEST_F(mathTest, functionsTest) {
    ASSERT_EQ(spider::math::saturateAdd(UINT32_MAX, static_cast<uint32_t>(1)), UINT32_MAX)
                                << "math::saturateAdd(uint32_t) failed";
    ASSERT_EQ(spider::math::saturateAdd(UINT32_MAX - 10, static_cast<uint32_t>(10)), UINT32_MAX)
                                << "math::saturateAdd(uint32_t) failed";
    ASSERT_EQ(spider::math::saturateAdd(UINT64_MAX, static_cast<uint64_t>(1)), UINT64_MAX)
                                << "math::saturateAdd(uint64_t) failed";
    ASSERT_EQ(spider::math::saturateAdd(UINT64_MAX - 10, static_cast<uint64_t>(10)), UINT64_MAX)
                                << "math::saturateAdd(uint64_t) failed";
    ASSERT_EQ(spider::math::saturateAdd(static_cast<uint64_t>(31415926535), static_cast<uint64_t>(10)), 31415926545u)
                                << "math::saturateAdd(uint64_t) failed";


    ASSERT_EQ(spider::math::abs(static_cast<int16_t>(-2)), 2) << "math::abs(int16_t) failed";
    ASSERT_EQ(spider::math::abs(static_cast<int32_t>(-2)), 2) << "math::abs(int32_t) failed";
    ASSERT_EQ(spider::math::abs(static_cast<int64_t>(-2)), 2) << "math::abs(int64_t) failed";
    ASSERT_EQ(spider::math::abs(static_cast<int16_t>(2)), 2) << "math::abs(int16_t) failed";
    ASSERT_EQ(spider::math::abs(static_cast<int32_t>(2)), 2) << "math::abs(int32_t) failed";
    ASSERT_EQ(spider::math::abs(static_cast<int64_t>(2)), 2) << "math::abs(int64_t) failed";
    ASSERT_EQ(spider::math::abs(static_cast<uint16_t>(2)), 2u) << "math::abs(uint16_t) failed";
    ASSERT_EQ(spider::math::abs(static_cast<uint32_t>(2)), 2u) << "math::abs(uint32_t) failed";
    ASSERT_EQ(spider::math::abs(static_cast<uint64_t>(2)), 2u) << "math::abs(uint64_t) failed";
    ASSERT_EQ(spider::math::abs(-2.f), 2.f) << "math::abs(float) failed";
    ASSERT_EQ(spider::math::abs(-2.), 2.) << "math::abs(double) failed";

    ASSERT_EQ(spider::math::gcd(static_cast<int64_t>(-4), static_cast<int64_t>(2)), 2) << "math::gcd() failed";
    ASSERT_EQ(spider::math::gcd(static_cast<int64_t>(4), static_cast<int64_t>(-2)), 2) << "math::gcd() failed";
    ASSERT_EQ(spider::math::gcd(static_cast<uint64_t>(4), static_cast<uint64_t>(2)), 2u) << "math::gcd() failed";
    ASSERT_EQ(spider::math::gcd(static_cast<int32_t>(-4), static_cast<int32_t>(2)), 2) << "math::gcd() failed";
    ASSERT_EQ(spider::math::gcd(static_cast<int32_t>(4), static_cast<int32_t>(-2)), 2) << "math::gcd() failed";
    ASSERT_EQ(spider::math::gcd(static_cast<uint32_t>(4), static_cast<uint32_t>(2)), 2u) << "math::gcd() failed";

    ASSERT_EQ(spider::math::lcm(static_cast<int64_t>(-4), static_cast<int64_t>(2)), 4) << "math::lcm() failed";
    ASSERT_EQ(spider::math::lcm(static_cast<int64_t>(4), static_cast<int64_t>(-2)), 4) << "math::lcm() failed";
    ASSERT_EQ(spider::math::lcm(static_cast<uint64_t>(4), static_cast<uint64_t>(2)), 4u) << "math::lcm() failed";
    ASSERT_EQ(spider::math::lcm(static_cast<int32_t>(-4), static_cast<int32_t>(2)), 4) << "math::lcm() failed";
    ASSERT_EQ(spider::math::lcm(static_cast<int32_t>(4), static_cast<int32_t>(-2)), 4) << "math::lcm() failed";
    ASSERT_EQ(spider::math::lcm(static_cast<uint32_t>(4), static_cast<uint32_t>(2)), 4u) << "math::lcm() failed";

    ASSERT_EQ(spider::math::ceilDiv(static_cast<uint64_t>(5), static_cast<uint64_t>(2)), 3u) << "math::ceilDiv() failed";
    ASSERT_EQ(spider::math::ceilDiv(5, 2), 3) << "math::ceilDiv() failed";
    ASSERT_EQ(spider::math::ceilDiv(-5, 2), -2) << "math::ceilDiv() failed";
    ASSERT_EQ(spider::math::ceilDiv(5, -2), -2) << "math::ceilDiv() failed";
    ASSERT_EQ(spider::math::ceilDiv(static_cast<int64_t>(5), static_cast<int64_t>(2)), 3) << "math::ceilDiv() failed";
    ASSERT_EQ(spider::math::ceilDiv(static_cast<int64_t>(-5), static_cast<int64_t>(2)), -2) << "math::ceilDiv() failed";
    ASSERT_EQ(spider::math::ceilDiv(static_cast<int64_t>(5), static_cast<int64_t>(-2)), -2) << "math::ceilDiv() failed";

    ASSERT_EQ(spider::math::floorDiv(5, 2), 2) << "math::floorDiv() failed";
    ASSERT_EQ(spider::math::floorDiv(-5, 2), -3) << "math::floorDiv() failed";
    ASSERT_EQ(spider::math::floorDiv(static_cast<int64_t>(5), static_cast<int64_t>(2)), 2) << "math::floorDiv() failed";
    ASSERT_EQ(spider::math::floorDiv(static_cast<int64_t>(-5), static_cast<int64_t>(2)), -3)
                                << "math::floorDiv() failed";
    ASSERT_EQ(spider::math::floorDiv(static_cast<int64_t>(5), static_cast<int64_t>(-2)), -3)
                                << "math::floorDiv() failed";
    ASSERT_EQ(spider::math::floorDiv(static_cast<uint64_t>(5), static_cast<uint64_t>(2)), 2u)
                                << "math::floorDiv() failed";

    try {
        throwSpiderException(
                "Sagax discreta glabro notarius impium Constanti occultas quidam impium militares infudit quos quosdam crimina sub Magnentio est sub infudit conplurium membra multa eminebat facinus in supergressus Hispania modo crimina crimina licentius modo Magnentio odorandi membra a occultas quos a consarcinando ferebatur occultas longe ingenuorum conspirasse est Hispania Paulus obterens cum perquam iussa tempus");
    } catch (spider::Exception &e) {
        e.what();
    }
}


TEST_F(mathTest, rationalTest) {
    ASSERT_DOUBLE_EQ(spider::Rational(1, 2).toDouble(), 0.5) << "Rational::toDouble() failed";
    ASSERT_EQ(spider::Rational(1, 2).toInt64(), 0) << "Rational::toInt64() failed";
    ASSERT_EQ(spider::Rational(314159265358979).toInt64(), 314159265358979) << "Rational::toInt64() failed";
    ASSERT_EQ(spider::Rational(314159265358979).toUInt64(), 314159265358979u) << "Rational::toUInt64() failed";
    ASSERT_EQ(spider::Rational(1, 2) == spider::Rational(1, 2), true) << "Rational::operator==() failed";
    ASSERT_EQ(spider::Rational(1, 2) > spider::Rational(1, 4), true) << "Rational::operator>() failed";
    ASSERT_EQ(spider::Rational(1, 2) >= spider::Rational(1, 4), true) << "Rational::operator>=() failed";
    ASSERT_EQ(spider::Rational(1, 2) >= spider::Rational(1, 2), true) << "Rational::operator>=() failed";
    ASSERT_EQ(spider::Rational(-1, 2) < spider::Rational(1, 4), true) << "Rational::operator<() failed";
    ASSERT_EQ(spider::Rational(-1, 2) <= spider::Rational(1, 4), true) << "Rational::operator<=() failed";
    ASSERT_EQ(spider::Rational(-1, 2) <= spider::Rational(-1, 2), true) << "Rational::operator<=() failed";
    ASSERT_EQ(spider::Rational(-1, 2).abs(), spider::Rational(1, 2)) << "Rational::abs() failed";
    ASSERT_EQ(spider::Rational(-1, 2).denominator(), 2) << "Rational::denominator() failed";
    ASSERT_EQ(spider::Rational(-1, 2).nominator(), -1) << "Rational::nominator() failed";
    ASSERT_EQ(spider::Rational(10, 20), spider::Rational(1, 2)) << "Rational::reduce() failed";
    ASSERT_EQ(!spider::Rational(0, 20), true) << "Rational::operator!() failed";
    ASSERT_EQ((spider::Rational(1, 20) ? 1 : 0), 1) << "Rational::operator bool() failed";
    ASSERT_EQ(spider::Rational(1, 20) > 0., true) << "Rational::operator>() failed";
    ASSERT_EQ(spider::Rational(1, 20) >= 0., true) << "Rational::operator>=() failed";
    ASSERT_EQ(spider::Rational(1, 4) < 0.5, true) << "Rational::operator<() failed";
    ASSERT_EQ(spider::Rational(1, 4) <= 0.25, true) << "Rational::operator<() failed";
    ASSERT_EQ(spider::Rational(1, 20) / spider::Rational(2), spider::Rational(1, 40)) << "Rational::operator/() failed";
    ASSERT_EQ(spider::Rational(1, 20) * spider::Rational(2), spider::Rational(1, 10)) << "Rational::operator*() failed";
    ASSERT_EQ(spider::Rational(1, 20) - spider::Rational(2), spider::Rational(-39, 20))
                                << "Rational::operator-() failed";
    ASSERT_EQ(spider::Rational(1, 20) + spider::Rational(2), spider::Rational(41, 20))
                                << "Rational::operator+() failed";
    ASSERT_THROW(spider::Rational(1, 0), spider::Exception)
                                << "Rational::Rational() should throw for null denominator.";
    ASSERT_THROW(spider::Rational(5, 4) / 0, spider::Exception)
                                << "Rational::operator/() should throw for null denominator.";
    ASSERT_THROW(spider::Rational(1, 4) /= 0, spider::Exception)
                                << "Rational::operator/=() should throw for null denominator.";
    ASSERT_EQ(spider::Rational(1, 4) *= 2, spider::Rational(1, 2)) << "Rational::operator*=() failed.";
    ASSERT_EQ(spider::Rational(1, 4) * 2, spider::Rational(1, 2)) << "Rational::operator*() failed.";
    ASSERT_EQ(spider::Rational(1, 4) /= 2, spider::Rational(1, 8)) << "Rational::operator/=() failed.";
    ASSERT_EQ(spider::Rational(1, 4) -= 2, spider::Rational(-7, 4)) << "Rational::operator-=() failed.";
    ASSERT_EQ(spider::Rational(1, 4) += 2, spider::Rational(9, 4)) << "Rational::operator+=() failed.";
    ASSERT_EQ(spider::Rational(1, 4) - 2, spider::Rational(-7, 4)) << "Rational::operator-() failed.";
    ASSERT_EQ(spider::Rational(1, 4) + 2, spider::Rational(9, 4)) << "Rational::operator+() failed.";
    ASSERT_EQ(spider::Rational(1, 4) != spider::Rational(2), true) << "Rational::operator!=() failed.";
    ASSERT_EQ(spider::Rational(1, 4) != spider::Rational(2, 8), false) << "Rational::operator!=() failed.";
    ASSERT_EQ(spider::Rational(1, -4) != spider::Rational(2, -8), false) << "Rational::operator!=() failed.";
}
