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
#include <memory/memory.h>
#include <containers/array.h>
#include <containers/vector.h>
#include <containers/stack.h>
#include <containers/deque.h>
#include <containers/queue.h>
#include <containers/map.h>
#include <containers/set.h>
#include <containers/unordered_map.h>
#include <containers/unordered_set.h>
#include <containers/forward_list.h>
#include <containers/list.h>
#include <api/spider.h>

class containersTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::start();
    }

    void TearDown() override {
        spider::quit();
    }
};

TEST_F(containersTest, arrayCtorTest) {
    ASSERT_NO_THROW(spider::array<double>()) << "spider::array() failed.";
    ASSERT_NO_THROW(spider::array<double>(10)) << "spider::array(size_t) failed.";
    ASSERT_NO_THROW(spider::array<double>(10, 3.1415)) << "spider::array(size_t, const T&) failed.";
    ASSERT_NO_THROW(spider::array<double>(0)) << "spider::array(size_t = 0) failed.";
    ASSERT_NO_THROW(spider::array<double>(0, 3.1415)) << "spider::array(size_t = 0, const T&) failed.";
    ASSERT_NO_THROW((spider::array<double>{0., 3.1415})) << "spider::array(initializer_list) failed.";
    auto arr = spider::array<double>(10, 3.1415);
    ASSERT_NO_THROW(auto test_cpy(arr)) << "spider::array(const spider::array&) failed.";
    auto test(arr);
    ASSERT_EQ(std::equal(test.begin(), test.end(), arr.begin()), true)
                                << "Copy construction of std::array does not result in copy.";
    ASSERT_NO_THROW(auto test_mv(std::move(arr))) << "spider::array(spider::array &&) failed.";
}

double test_const_at(const spider::array<double> &test, bool thrw) {
    if (thrw) {
        return test.at(10);
    } else {
        return test.at(4);
    }
}

TEST_F(containersTest, arrayAssignTest) {
    auto arr = spider::array<double>(10, 3.1415);
    ASSERT_NO_THROW(arr[8] = 3.1415) << "operator[] failed for spider::array.";
    ASSERT_NO_THROW(arr[6]) << "const operator[] failed for spider::array.";
    ASSERT_NO_THROW(arr.at(8) = 3.1415) << "method at() failed for spider::array.";
    ASSERT_THROW(arr.at(10) = 3.1415, std::out_of_range)
                                << "method at() should throw for out_of_bound index spider::array.";
    ASSERT_THROW(arr.at(SIZE_MAX) = 3.1415, std::out_of_range)
                                << "method at() should throw for out_of_bound index spider::array.";
    ASSERT_THROW(test_const_at(arr, true), std::out_of_range)
                                << "const method at() should throw for out_of_bound index spider::array";
//    ASSERT_DOUBLE_EQ(arr[0], 3.1415) << "failed value initialized spider::array";
//    ASSERT_DOUBLE_EQ(test_const_at(arr, false), 3.1415) << "const method at() should work for spider::array";
    ASSERT_EQ(arr.size(), 10u) << "invalid size for spider::array";
    spider::array<double> test;
    ASSERT_NO_THROW(test = spider::array<double>(2)) << "move assignment failed for spider::array";
    spider::array<double> test2{2., 2.};
    ASSERT_NO_THROW(test = test2);
    ASSERT_EQ(test, test2);
    ASSERT_NE((spider::array<double>{2., 2., 3.}), test2);
    ASSERT_NE((spider::array<double>{2., 3.}), test2);
    ASSERT_EQ(*(test.data()), *test.begin());
    ASSERT_NO_THROW(test.assign(3.14));
    ASSERT_NO_THROW(test.assign({}));
    ASSERT_NO_THROW(test.assign({1., 2., 3., 4.}));
    ASSERT_EQ((spider::array<double>{1., 2.}), test);
    ASSERT_EQ(test.back(), 2.);
    ASSERT_EQ(test.front(), 1.);
}

TEST_F(containersTest, arrayIteratorTest) {
    auto arr = spider::array<double>(10);
    double count = 1.;
    for (auto &val : arr) {
        val = 3.1415926535 + count;
        count += 1;
    }
    ASSERT_DOUBLE_EQ(*arr.begin(), 4.1415926535) << "begin() iterator of spider::array not pointing to proper value";
    ASSERT_DOUBLE_EQ(*(arr.end() - 1), 13.1415926535) << "end() iterator of spider::array not pointing to proper value";
    count = 1;
    for (const auto &val : arr) {
        ASSERT_DOUBLE_EQ(val, 3.1415926535 + count) << "Failed to set value through iterators spider::array";
        count += 1;
    }
}

TEST_F(containersTest, stdContainersCtorTest) {

    /* === Testing wrapper around std containers === */

    /* == spider::vector == */
    ASSERT_NO_THROW(spider::factory::vector<double>()) << "spider::factory::vector() failed.";
    ASSERT_NO_THROW(spider::factory::vector<double>(10)) << "spider::factory::vector(size_t) failed.";
    ASSERT_NO_THROW(spider::factory::vector<double>(10, 0.))
                                << "spider::factory::vector(size_t, const T &) failed.";
    auto tmp_vector = spider::factory::vector<double>();
    ASSERT_NO_THROW(spider::factory::vector<double>(tmp_vector))
                                << "spider::factory::vector(const spider::vector &) failed.";
    ASSERT_NO_THROW(spider::factory::vector<double>(spider::vector<double>()))
                                << "spider::factory::vector(spider::vector &&) failed.";
    ASSERT_NO_THROW(spider::factory::vector<double>({ 10, 0., 3.1415 }))
                                << "spider::factory::vector(std::initializer_list) failed.";

    /* == spider::deque == */
    ASSERT_NO_THROW(spider::factory::deque<double>()) << "spider::factory::deque() failed.";
    ASSERT_NO_THROW(spider::factory::deque<double>(10)) << "spider::factory::deque(size_t) failed.";
    ASSERT_NO_THROW(spider::factory::deque<double>(10, 0.))
                                << "spider::factory::deque(size_t, const T &) failed.";
    auto tmp_deque = spider::factory::deque<double>();
    ASSERT_NO_THROW(spider::factory::deque<double>(tmp_deque))
                                << "spider::factory::deque(const spider::deque &) failed.";
    ASSERT_NO_THROW(spider::factory::deque<double>(spider::deque<double>()))
                                << "spider::factory::deque(spider::deque &&) failed.";
    ASSERT_NO_THROW(spider::factory::deque<double>({ 10, 0., 3.1415 }))
                                << "spider::factory::deque(std::initializer_list) failed.";

    /* == spider::forward_list == */
    ASSERT_NO_THROW(spider::factory::forward_list<double>()) << "spider::factory::forward_list() failed.";
    ASSERT_NO_THROW(spider::factory::forward_list<double>(10, 0.))
                                << "spider::factory::forward_list(size_t, const T &) failed.";
    auto tmp_fwlist = spider::factory::forward_list<double>();
    ASSERT_NO_THROW(spider::factory::forward_list<double>(tmp_fwlist))
                                << "spider::factory::forward_list(const spider::forward_list &) failed.";
    ASSERT_NO_THROW(spider::factory::forward_list<double>(spider::forward_list<double>()))
                                << "spider::factory::forward_list(spider::forward_list &&)";

    /* == spider::list == */
    ASSERT_NO_THROW(spider::factory::list<double>()) << "spider::factory::list() failed.";
    ASSERT_NO_THROW(spider::factory::list<double>(10, 0.)) << "spider::factory::list(size_t, const T &) failed.";
    auto tmp_list = spider::factory::list<double>();
    ASSERT_NO_THROW(spider::factory::list<double>(tmp_list))
                                << "spider::factory::list(const spider::list &) failed.";
    ASSERT_NO_THROW(spider::factory::list<double>(spider::list<double>()))
                                << "spider::factory::list(spider::list &&) failed.";

    /* == spider::set == */
    ASSERT_NO_THROW(spider::factory::set<double>()) << "spider::factory::set() failed.";
    auto tmp_set = spider::factory::set<double>();
    ASSERT_NO_THROW(spider::factory::set<double>(tmp_set)) << "spider::factory::set(const spider::set &) failed.";
    ASSERT_NO_THROW(spider::factory::set<double>(spider::set<double>()))
                                << "spider::factory::set(spider::set &&) failed.";

    /* == spider::map == */
    ASSERT_NO_THROW((spider::factory::map<double, int>())) << "spider::factory::map() failed.";
    auto tmp_map = spider::factory::map<double, int>();
    ASSERT_NO_THROW((spider::factory::map(tmp_map))) << "spider::factory::map(const spider::map &) failed.";
    ASSERT_NO_THROW((spider::factory::map(spider::map<double, int>())))
                                << "spider::factory::map(spider::map &&) failed.";

    /* == spider::unordered_set == */
    ASSERT_NO_THROW(spider::factory::unordered_set<double>()) << "spider::factory::unordered_set() failed.";
    auto tmp_unordered_set = spider::factory::unordered_set<double>();
    ASSERT_NO_THROW(spider::factory::unordered_set(tmp_unordered_set))
                                << "spider::factory::unordered_set(const spider::unordered_set &) failed.";
    ASSERT_NO_THROW(spider::factory::unordered_set(spider::unordered_set<double>()))
                                << "spider::factory::unordered_set(spider::unordered_set &&) failed.";

    /* == spider::unordered_map == */
    ASSERT_NO_THROW((spider::factory::unordered_map<double, int>()))
                                << "spider::factory::unordered_map() failed.";
    auto tmp_unordered_map = spider::factory::unordered_map<double, int>();
    ASSERT_NO_THROW((spider::factory::unordered_map(tmp_unordered_map)))
                                << "spider::factory::unordered_map(const spider::unordered_map &) failed.";
    ASSERT_NO_THROW((spider::factory::unordered_map(spider::unordered_map<double, int>())))
                                << "spider::factory::unordered_map(spider::unordered_map &&) failed.";
}