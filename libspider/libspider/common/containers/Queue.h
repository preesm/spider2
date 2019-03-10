/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2018)
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

#ifndef SPIDER2_QUEUE_H
#define SPIDER2_QUEUE_H

#include <queue>
#include <mutex>
#include <semaphore.h>

/**
 * @brief Thread safe and passive implementation of std::queue (using semaphore to wake waiting thread on queue)
 */
template<typename T>
class Queue {
public:
    /**
     * @brief Constructor
     */
    Queue();

    /**
     * @brief Destructor
     */
    ~Queue();

    /**
     * @brief Push a value to the queue. (thread safe)
     *
     * @param value Pointer to the value to be pushed
     */
    void push(T *data);

    /**
     * @brief Read one value in the queue if it contains any. (thread safe)
     *
     * @param data      Pointer to data to be filled with queue content.
     * @param blocking  Flag to wait (true) if queue is empty or return (false).
	 * @return false if queue was empty, true if succesfully poped
     */
    bool pop(T *data, bool blocking);

    /**
     * @brief Clear the queue (thread safe)
     */
    void clear();

	/**
     * @brief Return size of the queue at given instant (thread safe)
     *
	 * @return false if queue was empty, true if succesfully poped
     */
	 inline std::uint64_t size() const;

private:
    std::uint64_t queueSize_;
    std::queue<T> queue_;
    std::mutex queueMutex_;
    sem_t queueCounter_;
};

template<typename T>
std::uint64_t Queue<T>::size() const {
	/*!< Locking the mutex to avoid returning non valid size */
    std::lock_guard<std::mutex> lock(queueMutex_);
	return queueSize_;
}

#endif //SPIDER2_QUEUE_H
