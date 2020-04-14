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
#ifndef SPIDER2_RATIONAL_H_
#define SPIDER2_RATIONAL_H_

/* === Includes === */

#include <cstdint>
#include <common/Math.h>
#include <common/Exception.h>

/* === Class definition === */

namespace spider {

    class Rational {
    public:
        inline explicit Rational(int64_t n = 0, int64_t d = 1) : n_{ n }, d_{ d } {
            if (d_ == 0) {
                throwSpiderException("Fraction with zero denominator not allowed.");
            }
            reduce();
        };

        inline Rational(const Rational &r) : Rational{ r.n_, r.d_ } { };

        /* === Operators overload === */

        inline Rational &operator+=(const Rational &b) {
            n_ = (n_ * b.d_) + (b.n_ * d_);
            d_ *= b.d_;
            reduce();
            return *this;
        }

        inline Rational &operator-=(const Rational &b) {
            n_ = (n_ * b.d_) - (b.n_ * d_);
            d_ *= b.d_;
            reduce();
            return *this;
        }

        inline Rational &operator*=(const Rational &b) {
            n_ *= b.n_;
            d_ *= b.d_;
            reduce();
            return *this;
        }

        inline Rational &operator/=(const Rational &b) {
            if (!b.n_) {
                throwSpiderException("Fraction with zero denominator not allowed.");
            }
            n_ *= b.d_;
            d_ *= b.n_;
            reduce();
            return *this;
        }

        inline Rational operator+(const Rational &b) const {
            return Rational{ *this } += b;
        }

        inline Rational operator-(const Rational &b) const {
            return Rational{ *this } -= b;
        }

        inline Rational operator*(const Rational &b) const {
            return Rational{ *this } *= b;
        }

        inline Rational operator/(const Rational &b) const {
            return Rational{ *this } /= b;
        }

        inline bool operator==(const Rational &b) const {
            return n_ == b.n_ && d_ == b.d_;
        }

        inline bool operator!=(const Rational &b) const {
            return !(*this == b);
        }

        inline bool operator!() const {
            return n_ == 0;
        }

        inline explicit operator bool() const {
            return n_ != 0;
        }

        inline bool operator>(const Rational &b) const {
            auto diff = *this - b;
            return diff.n_ > 0;
        }

        inline bool operator<(const Rational &b) const {
            auto diff = *this - b;
            return diff.n_ < 0;
        }

        inline bool operator>=(const Rational &b) const {
            return !(*this < b);
        }

        inline bool operator<=(const Rational &b) const {
            return !(*this > b);
        }

        /* === Overloads === */

        inline Rational operator+(int64_t b) const {
            return Rational{ n_ + b * d_, d_ };
        }

        inline Rational operator-(int64_t b) const {
            return Rational{ n_ - b * d_, d_ };
        }

        inline Rational operator*(int64_t b) const {
            return Rational{ n_ * b, d_ };
        }

        inline Rational operator/(int64_t b) const {
            return Rational{ n_, d_ * b };
        }

        inline Rational &operator+=(int64_t b) {
            n_ += (b * d_);
            reduce();
            return *this;
        }

        inline Rational &operator-=(int64_t b) {
            n_ -= (b * d_);
            reduce();
            return *this;
        }

        inline Rational &operator*=(int64_t b) {
            n_ *= b;
            reduce();
            return *this;
        }

        inline Rational &operator/=(int64_t b) {
            if (!b) {
                throwSpiderException("Fraction with zero denominator not allowed.");
            }
            d_ *= b;
            reduce();
            return *this;
        }

        inline bool operator>(double b) const {
            return this->toDouble() > b;
        }

        inline bool operator<(double b) const {
            return this->toDouble() < b;
        }

        inline bool operator>=(double b) const {
            return !(*this < b);
        }

        inline bool operator<=(double b) const {
            return !(*this > b);
        }

        /* === Methods === */

        inline Rational abs() const {
            return Rational{ math::abs((*this).n_), math::abs((*this).d_) };
        }

        inline int64_t toInt64() const {
            return n_ / d_;
        }

        inline uint64_t toUInt64() const {
            return static_cast<uint64_t >(n_ / d_);
        }

        inline double toDouble() const {
            return static_cast<double >(n_) / static_cast<double >(d_);
        }

        /* === Getters === */

        inline int64_t denominator() const {
            return d_;
        }

        inline int64_t nominator() const {
            return n_;
        }

    private:
        int64_t n_;
        int64_t d_;

        /* === Private method(s) === */

        inline void reduce() {
            if (d_ < 0) {
                n_ = -n_;
                d_ = -d_;
            }
            auto gcd = math::gcd(n_, d_);
            n_ /= gcd;
            d_ /= gcd;
        }
    };
}
#endif // SPIDER2_RATIONAL_H_
