/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2014 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2014 - 2015)
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
#ifndef SPIDER2_RATIONAL_H_
#define SPIDER2_RATIONAL_H_

/* === Includes === */

#include <cstdint>
#include <iostream>
#include <common/Math.h>
#include <common/Exception.h>

/* === Class definition === */

namespace Spider {

    class Rational {
    public:
        inline explicit Rational(std::int64_t n = 0, std::int64_t d = 1);

        inline Rational(const Rational &r);

        /* === Operators overload === */

        inline Rational &operator+=(const Rational &b);

        inline Rational &operator-=(const Rational &b);

        inline Rational &operator*=(const Rational &b);

        inline Rational &operator/=(const Rational &b);

        inline Rational operator+(const Rational &b) const;

        inline Rational operator-(const Rational &b) const;

        inline Rational operator*(const Rational &b) const;

        inline Rational operator/(const Rational &b) const;

        inline Rational operator+(const std::int64_t &b) const;

        inline Rational operator-(const std::int64_t &b) const;

        inline Rational operator*(const std::int64_t &b) const;

        inline Rational operator/(const std::int64_t &b) const;

        inline bool operator==(const Rational &b) const;

        inline bool operator==(const std::int64_t &a) const;

        inline bool operator!=(const Rational &b) const;

        inline bool operator!=(const std::int64_t &a) const;

        inline bool operator!() const;

        inline explicit operator bool() const;

        inline bool operator>(const Rational &b) const;

        inline bool operator<(const Rational &b) const;

        inline bool operator>=(const Rational &b) const;

        inline bool operator<=(const Rational &b) const;

        /* === Methods === */

        inline Rational abs() const;

        inline std::int64_t toInt64() const;

        inline std::int32_t toInt32() const;

        inline double toDouble() const;

        /* === Getters === */

        inline std::int64_t denominator() const;

        inline std::int64_t nominator() const;

        friend std::ostream &operator<<(std::ostream &, const Rational &);

    private:
        std::int64_t n_;
        std::int64_t d_;

        /* === Private method(s) === */

        inline void reduce();
    };

    /* === Inline method(s) === */

    inline std::ostream &operator<<(std::ostream &stream, const Rational &r) {
        stream << "[" << r.nominator() << " / " << r.denominator() << "]";
        return stream;
    }

    Rational::Rational(int64_t n, int64_t d) : n_{n}, d_{d} {
        if (d_ == 0) {
            throwSpiderException("Fraction with zero denominator not allowed.");
        }
        reduce();
    }

    Rational::Rational(const Rational &r) : Rational{r.n_, r.d_} {

    }

    Rational &Rational::operator+=(const Rational &b) {
        n_ = (n_ * b.d_) + (b.n_ * d_);
        d_ *= b.d_;
        reduce();
        return *this;
    }

    Rational &Rational::operator-=(const Rational &b) {
        n_ = (n_ * b.d_) - (b.n_ * d_);
        d_ *= b.d_;
        reduce();
        return *this;
    }

    Rational &Rational::operator*=(const Rational &b) {
        n_ *= b.n_;
        d_ *= b.d_;
        reduce();
        return *this;
    }

    Rational &Rational::operator/=(const Rational &b) {
        if (b.n_ == 0) {
            throwSpiderException("Fraction with zero denominator not allowed.");
        }
        n_ *= b.d_;
        d_ *= b.n_;
        reduce();
        return *this;
    }

    Rational Rational::operator+(const Rational &b) const {
        return Rational{*this} += b;
    }

    Rational Rational::operator-(const Rational &b) const {
        return Rational{*this} -= b;
    }

    Rational Rational::operator*(const Rational &b) const {
        return Rational{*this} *= b;
    }

    Rational Rational::operator/(const Rational &b) const {
        return Rational{*this} /= b;
    }

    Rational Rational::operator+(const std::int64_t &b) const {
        return *this + Rational{b};
    }

    Rational Rational::operator-(const std::int64_t &b) const {
        return *this - Rational{b};
    }

    Rational Rational::operator*(const std::int64_t &b) const {
        return *this * Rational{b};
    }

    Rational Rational::operator/(const std::int64_t &b) const {
        return *this / Rational{b};
    }

    bool Rational::operator==(const Rational &b) const {
        return n_ == b.n_ && d_ == b.d_;
    }

    bool Rational::operator==(const std::int64_t &a) const {
        return this->toDouble() == static_cast<double >(a);
    }

    bool Rational::operator!=(const Rational &b) const {
        return !(*this == b);
    }

    bool Rational::operator!=(const std::int64_t &a) const {
        return !(*this == a);
    }

    bool Rational::operator!() const {
        return n_ == 0;
    }

    Rational::operator bool() const {
        return n_ != 0;
    }

    bool Rational::operator>(const Rational &b) const {
        auto diff = *this - b;
        return diff.n_ > 0;
    }

    bool Rational::operator<(const Rational &b) const {
        auto diff = *this - b;
        return diff.n_ < 0;
    }

    bool Rational::operator>=(const Rational &b) const {
        return !(*this < b);
    }

    bool Rational::operator<=(const Rational &b) const {
        return !(*this > b);
    }

    std::int64_t Rational::toInt64() const {
        return static_cast<std::int64_t >(n_ / d_);
    }

    std::int32_t Rational::toInt32() const {
        return static_cast<std::int32_t >(n_ / d_);
    }

    double Rational::toDouble() const {
        return static_cast<double >(n_) / static_cast<double >(d_);
    }

    Rational Rational::abs() const {
        return Rational{Spider::Math::abs((*this).n_), Spider::Math::abs((*this).d_)};
    }

    std::int64_t Rational::denominator() const {
        return d_;
    }

    std::int64_t Rational::nominator() const {
        return n_;
    }

    void Rational::reduce() {
        auto gcd = Spider::Math::gcd(n_, d_);
        n_ /= gcd;
        d_ /= gcd;
        if (d_ < 0) {
            n_ = -n_;
            d_ = -d_;
        }
    }
}
#endif // SPIDER2_RATIONAL_H_
