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

#include <thread/Thread.h>

#ifdef _WIN32

#include <windows.h>
#include <process.h>

#elif defined __APPLE__

#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <common/Logger.h>

#else

#include <pthread.h>
#include <sched.h>

#endif

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

#ifdef _WIN32

#define MASK(id) (0x00000001 << affinity_id)

bool spider::thread::set_affinity(std::int32_t affinity_id) {
    auto ret = SetThreadAffinityMask(spider::this_thread::native_handle(), MASK(static_cast<std::uint32_t>(affinity_id)));
    if (ret != 0) {
        return true;
    }
    return false;
}

#elif (defined __APPLE__)

bool spider::thread::set_affinity(std::int32_t affinity_id) {
    spider::log::warning("Thread affinity is not guaranteed on OSX platform..\n");
    auto mac_thread = pthread_mach_thread_np(spider::this_thread::native_handle());
    thread_affinity_policy_data_t policyData = { affinity_id };
    auto ret = thread_policy_set(mach_thread1, THREAD_AFFINITY_POLICY, (thread_policy_t) &policyData, 1);
    if (ret == KERN_SUCCESS) {
        return true;
    }
    return false;
}

#elif (defined __linux__ || defined BSD4_4) && !(defined ANDROID)

bool spider::thread::set_affinity(std::int32_t affinity_id) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(affinity_id, &cpu_set);
    auto ret = pthread_setaffinity_np(spider::this_thread::native_handle(), sizeof(cpu_set_t), &cpu_set);
    if (ret != 0) {
        return true;
    }
    std::this_thread::get_id();
    return false;
}

#endif

#ifdef _WIN32

std::thread::native_handle_type spider::this_thread::native_handle() {
    return GetCurrentThread();
}

#elif __APPLE__

std::thread::native_handle_type spider::this_thread::native_handle() {
    spider::log::warning("native_handle not supported on apple platform.\n");
    return 0;
}

#elif (defined __linux__ || defined BSD4_4) && !(defined ANDROID)

std::thread::native_handle_type spider::this_thread::native_handle() {
    return pthread_self();
}

#endif
