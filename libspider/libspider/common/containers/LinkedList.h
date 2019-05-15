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
#ifndef SPIDER2_LINKEDLIST_H
#define SPIDER2_LINKEDLIST_H

/* === Includes === */

#include <cstdint>
#include <common/memory/Allocator.h>

/* === Structure definition === */

/**
 * @brief Node used in @refitem LinkedList
 * @tparam T Template type of the value of the node.
 */
template<typename T>
struct NodeList {
    T value;
    NodeList *next = nullptr;
    NodeList *previous = nullptr;
};

/* === Class definition === */

template<typename T>
class LinkedList {
public:
    explicit inline LinkedList(StackID stack);

    inline ~LinkedList();

    /* === Operators === */

    NodeList<T> *operator[](std::uint32_t ix);

    /* === Methods === */

    /**
     * @brief Set on next node of the list and return it
     * @return  next element of the list
     */
    inline NodeList<T> *next();

    /**
     * @brief Set on previous node of the list and return it
     * @return  previous element of the list
     */
    inline NodeList<T> *previous();

    /**
     * @brief Set current to val.
     * @param val @refitem NodeList to set current on.
     */
    inline void setOnValue(NodeList<T> *val);

    /**
     * @brief Creates a new @refitem NodeList with value val and add it as head of the list.
     * @param val  Value to add.
     */
    inline void addHead(T val);

    /**
     * @brief Creates a new @refitem NodeList with value val and add it as tail of the list.
     * @param val  Value to add.
     */
    inline void addTail(T val);

    /**
     * @brief Creates a new @refitem NodeList with value val and add it after current in the list.
     * @param val  Value to add.
     */
    inline void addCurrent(T val);

    /**
     * @brief Test if the LinkedList contains a given value.
     * @param val  Value to test.
     * @return true if value is in the LinkedList, false else.
     */
    inline bool contains(T val) const;


    /**
     * @brief Remove a node in the LinkedList.
     * @param node NodeList to remove.
     */
    inline void remove(NodeList<T> *node);

    /* === Getters === */

    /**
     * @brief Return first node of the list. Current is unchanged.
     * @return first node of the list
     */
    inline NodeList<T> *head() const;

    /**
     * @brief Return last node of the list. Current is unchanged.
     * @return last node of the list
     */
    inline NodeList<T> *tail() const;

    /**
     * @brief Return current node of the list
     * @return current node of the list
     */
    inline NodeList<T> *current() const;

    /**
     * @brief Get the size of the LinkedList.
     * @return size of the list.
     */
    inline std::uint32_t size() const;

private:
    NodeList<T> *head_ = nullptr;
    NodeList<T> *current_ = nullptr;
    NodeList<T> *tail_ = nullptr;

    StackID stack_;
    std::uint32_t size_ = 0;

    inline NodeList<T> *newNodeList(T &val, NodeList<T> *prev = nullptr, NodeList<T> *next = nullptr) const;
};

/* === Inline methods === */


template<typename T>
LinkedList<T>::LinkedList(StackID stack) : stack_{stack} {

}

template<typename T>
LinkedList<T>::~LinkedList() {
    setOnValue(head_);
    while (size_) {
        remove(current_);
    }
}

template<class T>
inline NodeList<T> *LinkedList<T>::operator[](std::uint32_t ix) {
    if (ix >= size_) {
        throwSpiderException("Accesing unitialized element. Ix = %"
                                     PRIu32
                                     " -- Size = %"
                                     PRIu32
                                     "", ix, size_);
    }
    std::uint32_t i = 0;
    auto *current = head_;
    while (i < ix) {
        current = current->next;
        i++;
    }
    return current;
}

template<typename T>
inline NodeList<T> *LinkedList<T>::next() {
    current_ = current_->next;
    return current_;
}

template<typename T>
inline NodeList<T> *LinkedList<T>::previous() {
    current_ = current_->previous;
    return current_;
}

template<typename T>
void LinkedList<T>::setOnValue(NodeList<T> *val) {
    if (val) {
        current_ = val;
    }
}

template<typename T>
void LinkedList<T>::addHead(T val) {
    if (!head_) {
        head_ = newNodeList(val);
        tail_ = head_;
        current_ = head_;
        head_->next = head_;
        head_->previous = head_;
    } else {
        head_ = newNodeList(val, tail_, head_);
    }
    size_++;
}

template<typename T>
void LinkedList<T>::addTail(T val) {
    if (!head_) {
        tail_ = newNodeList(val);
        head_ = tail_;
        current_ = tail_;
        head_->next = tail_;
        head_->previous = tail_;
    } else {
        tail_ = newNodeList(val, tail_, head_);
    }
    size_++;
}

template<typename T>
void LinkedList<T>::addCurrent(T val) {
    if (!current_) {
        current_ = newNodeList(val);
        head_ = current_;
        tail_ = current_;
        current_->next = current_;
        current_->previous = current_;
    } else {
        current_ = newNodeList(val, current_, current_->next);
        if (current_->previous == tail_) {
            tail_ = current_;
        }
    }
    size_++;

}

template<typename T>
bool LinkedList<T>::contains(T val) const {
    auto *node = head_;
    do {
        if (node->value == val) {
            return true;
        }
        node = node->next;
    } while (node != tail_);
    return false;
}

template<typename T>
void LinkedList<T>::remove(NodeList<T> *node) {
    if (node && size_) {
        auto *previousNode = node->previous;
        auto *nextNode = node->next;
        if (node == current_) {
            current_ = nextNode;
        }
        if (node == head_) {
            head_ = nextNode;
        }
        if (node == tail_) {
            tail_ = previousNode;
        }
        nextNode->previous = previousNode;
        previousNode->next = nextNode;
        Allocator::deallocate(node);
        size_--;
        if (!size_) {
            head_ = current_ = tail_ = nullptr;
        }
    }
}

template<typename T>
NodeList<T> *LinkedList<T>::head() const {
    return head_;
}

template<typename T>
NodeList<T> *LinkedList<T>::tail() const {
    return tail_;
}

template<typename T>
NodeList<T> *LinkedList<T>::current() const {
    return current_;
}

template<typename T>
std::uint32_t LinkedList<T>::size() const {
    return size_;
}

/* === Private inline methods === */

/**
 * @brief Create a new @refitem NodeList.
 * @tparam T Template type
 * @param val Value
 * @param prev Previous NodeList
 * @param next Next NodeList
 * @return Newly created NodeList
 */
template<typename T>
NodeList<T> *LinkedList<T>::newNodeList(T &val, NodeList<T> *prev, NodeList<T> *next) const {
    auto *node = Allocator::allocate<NodeList<T> >(stack_);
    node->value = val;
    node->previous = prev;
    node->next = next;
    if (prev) {
        prev->next = node;
    }
    if (next) {
        next->previous = node;
    }
    return node;
}


#endif //SPIDER2_LINKEDLIST_H
