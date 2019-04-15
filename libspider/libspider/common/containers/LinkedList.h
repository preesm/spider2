/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2014 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2014 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2014)
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
#ifndef SPIDER_LINKEDLIST_H
#define SPIDER_LINKEDLIST_H

#include <common/memory/StackAllocator.h>
#include <common/SpiderException.h>

template<class T>
class LinkedList {
public:
    typedef struct Node {
        T val_;
        Node *prev_ = nullptr;
        Node *next_ = nullptr;
    } Node;

    LinkedList(SpiderStack stackId, std::int32_t sizeMax);

    ~LinkedList();

    T &operator[](int ix);

    /**
     * @brief Return first node of the list. Current is unchanged.
     * @return first node of the list
     */
    inline Node *first() const;

    /**
     * @brief Return last node of the list. Current is unchanged.
     * @return last node of the list
     */
    inline Node *last() const;

    /**
     * @brief Return current node of the list.
     * @return current node of the list
     */
    inline Node *current() const;

    /**
     * @brief Set current on next node of the list and return it.
     * @return  next element of the list
     */
    inline Node *next() const;

    /**
     * @brief Set current on previous node of the list and return it.
     * @return  previous element of the list
     */
    inline Node *previous() const;

    /**
     * @brief Set current as first.
     */
    void setOnFirst();

    /**
     * @brief Set current as last.
     */
    void setOnLast();

    /**
     * @brief Return current size of the list.
     * @return size of the list
     */
    inline std::int32_t size() const;

    /**
     * @brief Add an element in first position to the list.
     * @param e Element to be added.
     */
    void add(T e);

    /**
     * @brief Add an element in last position to the list.
     * @param e Element to be added.
     */
    void addLast(T e);

    /**
     * @brief Remove a node from the list.
     * @param n Node to be removed
     * @return value held by the removed node.
     */
    T &del(Node *n);

    /**
     * @brief Test if an element is in the list.
     * @param element Element to be tested.
     * @return true if element is in the list, false else
     */
    bool contains(T item);

private:
    Node *head_ = nullptr;
    Node *current_ = nullptr;
    Node *tail_ = nullptr;
    std::int32_t size_ = 0;
    SpiderStack stackId_;
    std::int32_t sizeMax_;
};

template<class T>
inline LinkedList<T>::LinkedList(SpiderStack stackId, std::int32_t sizeMax) : stackId_{stackId}, sizeMax_{sizeMax} {
}

template<class T>
inline LinkedList<T>::~LinkedList() {
    if (size_ > 0) {
        setOnFirst();
        auto *n = current_;
        while (n) {
            del(n);
            n = current_;
        }
    }
}

template<class T>
inline T &LinkedList<T>::operator[](int ix) {
    if (ix < 0 || ix >= size_) {
        throwSpiderException("Accesing unitialized element. Ix = %d -- Size = %d", ix, size_);
    }
    std::int32_t i = 0;
    auto *current = head_;
    while (i < ix) {
        current = current->next_;
        i++;
    }
    return current->val_;
}

template<class T>
inline typename LinkedList<T>::Node *LinkedList<T>::first() const {
    return head_;
}

template<class T>
inline typename LinkedList<T>::Node *LinkedList<T>::last() const {
    return tail_;
}

template<class T>
inline typename LinkedList<T>::Node *LinkedList<T>::current() const {
    return current_;
}

template<class T>
inline typename LinkedList<T>::Node *LinkedList<T>::next() const {
    current_ = current_->next_;
    return current_;
}

template<class T>
inline typename LinkedList<T>::Node *LinkedList<T>::previous() const {
    current_ = current_->prev_;
    return current_;
}

template<class T>
inline void LinkedList<T>::setOnFirst() {
    current_ = head_;
}

template<class T>
inline void LinkedList<T>::setOnLast() {
    current_ = tail_;
}

template<class T>
inline std::int32_t LinkedList<T>::size() const {
    return size_;
}

template<class T>
inline void LinkedList<T>::add(T e) {
    if (size_ >= sizeMax_) {
        throwSpiderException("Can not add element, list is full.");
    }
    auto *newNode = Allocator::allocate(sizeof(Node), stackId_);
    if (!head_) {
        head_ = newNode;
        tail_ = newNode;
    }
    newNode->val_ = e;
    newNode->prev_ = tail_;
    newNode->next_ = head_;
    head_->prev_ = newNode;
    tail_->next_ = newNode;
    head_ = newNode;
    size_++;
}

template<class T>
inline void LinkedList<T>::addLast(T e) {
    if (size_ >= sizeMax_) {
        throwSpiderException("Can not add element, list is full.");
    }
    auto *newNode = Allocator::allocate(sizeof(Node), stackId_);
    if (!head_) {
        head_ = newNode;
        tail_ = newNode;
    }
    newNode->val_ = e;
    newNode->prev_ = tail_;
    newNode->next_ = head_;
    head_->prev_ = newNode;
    tail_->next_ = newNode;
    tail_ = newNode;
    size_++;
}


template<class T>
inline bool LinkedList<T>::contains(T item) {
    auto *current = head_;
    while (current) {
        if (current->val_ == item) {
            return true;
        }
        current = current->next_;
    }
    return false;
}

template<class T>
inline T &LinkedList<T>::del(LinkedList::Node *n) {
    if (n) {
        size_--;
        if (size_ < 0) {
            throwSpiderException("Trying to remove node not in the list.");
        } else if (size_ == 0) {
            head_ = nullptr;
            tail_ = nullptr;
            current_ = nullptr;
        } else {
            auto *previousNode = n->prev_;
            auto *nextNode = n->next_;
            previousNode->next_ = nextNode;
            nextNode->prev_ = previousNode;
            if (n == current_) {
                current_ = nextNode;
            }
            if (n == head_) {
                head_ = nextNode;
            } else if (n == tail_) {
                tail_ = previousNode;
            }
        }
        auto &val = n->val_;
        Allocator::deallocate(n);
        return val;
    } else {
        throwSpiderException("Trying to remove nullptr node.");
    }
}


#endif/*LIST_H*/
