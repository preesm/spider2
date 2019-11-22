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
#ifndef SPIDER2_LINKEDLIST_H
#define SPIDER2_LINKEDLIST_H

/* === Includes === */

#include <cstdint>
#include <cassert>
#include <memory/allocator.h>

namespace spider {

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

        inline T &operator*() {
            return value;
        }

        inline T &operator->() {
            return value;
        }

        inline explicit operator T &() {
            return value;
        }

        inline explicit operator const T &() const {
            return value;
        }
    };

    /* === Iterator definition === */

    template<typename T, bool is_const>
    class LinkedListIterator final : public std::iterator<std::forward_iterator_tag, T> {
    public:
        /**
         * For const_iterator:   define DataStructurePointerType to be a   const NodeList<T> *
         * For regular iterator: define DataStructurePointerType to be a   NodeList<T> *
         */
        typedef typename std::conditional<is_const, const NodeList<T> *, NodeList<T> *>::type pointer_type;

        /**
         * For const_iterator:   define ValueReferenceType to be a   const ValueType&
         * For regular iterator: define ValueReferenceType to be a   ValueType&
         */
        typedef typename std::conditional<is_const, const T &, T &>::type iteratorValueType;

        LinkedListIterator() : itr{ nullptr } {

        }

        /**
         * @brief Regular constructor
         * @param tmp
         */
        LinkedListIterator(pointer_type tmp) : itr{ tmp } {

        }

        /**
         * @brief Copy constructor for implicit conversion from regular iterator to const_iterator
         * @param tmp
         */
        LinkedListIterator(const LinkedListIterator<T, false> &tmp) : itr{ tmp.itr } {

        }

        inline bool operator==(const LinkedListIterator &rhs) const {
            return itr == rhs.itr;
        }

        inline bool operator!=(const LinkedListIterator &rhs) const {
            return itr != rhs.itr;
        }

        inline iteratorValueType &operator*() {
            assert(itr != nullptr && "Invalid iterator dereference!");
            return itr->value;
        }

        inline iteratorValueType &operator->() {
            assert(itr != nullptr && "Invalid iterator dereference!");
            return itr->value;
        }

        /* == Post-increment == */
        inline LinkedListIterator<T, is_const> operator++(int) {
            assert(itr != nullptr && "Out-of-bounds iterator increment!");
            auto tmp = LinkedListIterator(*this);
            itr = itr->next;
            return tmp;
        }

        /* == Pre-increment == */
        inline const LinkedListIterator &operator++() {
            assert(itr != nullptr && "Out-of-bounds iterator increment!");
            itr = itr->next;
            return *this;
        };

        /* == Converting iterator to const_iterator == */
        friend class LinkedListIterator<T, true>;

    private:
        pointer_type itr;
    };

    /* === Class definition === */

    template<typename T>
    class LinkedList {

    public:
        LinkedList() = default;

        explicit inline LinkedList(StackID stack);

        LinkedList(const LinkedList &other, StackID stack = StackID::GENERAL);

        LinkedList(LinkedList &&other) noexcept;

        inline ~LinkedList();

        /* === Operators === */

        T &operator[](uint64_t ix);

        T &operator[](uint64_t ix) const;

        /**
         * @brief use the "making new friends idiom" from
         * https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Making_New_Friends
         * @param first   First element to swap.
         * @param second  Second element to swap.
         */
        inline friend void swap(LinkedList<T> &first, LinkedList<T> &second) noexcept {
            using std::swap;

            /* == Do the swapping of the values == */
            swap(first.head_, second.head_);
            swap(first.current_, second.current_);
            swap(first.tail_, second.tail_);
            swap(first.size_, second.size_);
            swap(first.stack_, second.stack_);
        }

        /**
         * @brief Assignment operator. Use move constructor if used with an rvalue reference, copy constructor else.
         * Use the copy and swap idiom.
         * @param temp
         * @return reference to this.
         */
        inline LinkedList &operator=(LinkedList temp);

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

        /**
         * @brief Remove the first @refitem NodeList whose value is equal to val.
         * @param val Value to remove.
         */
        inline void removeFromValue(T val);

        /* === Iterator methods === */

        typedef LinkedListIterator<T, false> iterator;
        typedef LinkedListIterator<T, true> const_iterator;

        iterator begin();

        iterator end();

        const_iterator begin() const;

        const_iterator end() const;

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
        inline uint64_t size() const;

    private:
        NodeList<T> *head_ = nullptr;
        NodeList<T> *current_ = nullptr;
        NodeList<T> *tail_ = nullptr;

        StackID stack_ = StackID::GENERAL;
        uint64_t size_ = 0;

        inline NodeList<T> *newNodeList(T &val, NodeList<T> *prev = nullptr, NodeList<T> *next = nullptr) const;
    };

    /* === Inline methods === */


    template<typename T>
    LinkedList<T>::LinkedList(StackID stack) : stack_{ stack } {

    }

    template<typename T>
    LinkedList<T>::LinkedList(const LinkedList &other, StackID stack) : size_{ 0 }, stack_{ stack } {
        for (auto &val : other) {
            addCurrent(val);
        }
        assert(size_ == other.size_ && "Copied LinkedList should have same size");
    }

    template<typename T>
    LinkedList<T>::LinkedList(LinkedList &&other) noexcept : LinkedList() {
        swap(*this, other);
    }

    template<typename T>
    LinkedList<T>::~LinkedList() {
        setOnValue(head_);
        while (size_) {
            remove(current_);
        }
    }

    template<class T>
    inline T &LinkedList<T>::operator[](uint64_t ix) {
        if (ix >= size_) {
            throwSpiderException("Accessing uninitialized element. Ix = %"
                                         PRIu64
                                         " -- Size = %"
                                         PRIu64
                                         "", ix, size_);
        }
        uint64_t i = 0;
        auto *current = head_;
        while (i < ix && current != tail_) {
            current = current->next;
            i++;
        }
        return current->value;
    }

    template<class T>
    inline T &LinkedList<T>::operator[](uint64_t ix) const {
        return operator[](ix);
    }

    template<typename T>
    LinkedList<T> &LinkedList<T>::operator=(LinkedList temp) {
        swap(*this, temp);
        return *this;
    }

    template<typename T>
    inline NodeList<T> *LinkedList<T>::next() {
        if (current_->next) {
            current_ = current_->next;
        }
        return current_;
    }

    template<typename T>
    inline NodeList<T> *LinkedList<T>::previous() {
        if (current_->previous) {
            current_ = current_->previous;
        }
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
            head_->next = nullptr;
            head_->previous = nullptr;
        } else {
            head_ = newNodeList(val, nullptr, head_);
        }
        size_++;
    }

    template<typename T>
    void LinkedList<T>::addTail(T val) {
        if (!head_) {
            tail_ = newNodeList(val);
            head_ = tail_;
            current_ = tail_;
            head_->next = nullptr;
            head_->previous = nullptr;
        } else {
            tail_ = newNodeList(val, tail_, nullptr);
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
            if (nextNode) {
                nextNode->previous = previousNode;
            } else {
                current_ = tail_;
            }
            if (previousNode) {
                previousNode->next = nextNode;
            } else {
                current_ = head_;
            }
            spider::deallocate(node);
            size_--;
            if (!size_) {
                head_ = current_ = tail_ = nullptr;
            }
        }
    }

    template<typename T>
    void LinkedList<T>::removeFromValue(T val) {
        auto *node = head_;
        do {
            if (node->value == val) {
                remove(node);
                return;
            }
            node = node->next;
        } while (node != head_);
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
    uint64_t LinkedList<T>::size() const {
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
        auto *node = spider::allocate<NodeList<T> >(stack_);
        spider::construct(node);
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

    template<typename T>
    typename LinkedList<T>::iterator LinkedList<T>::begin() {
        return iterator(head_);
    }

    template<typename T>
    typename LinkedList<T>::iterator LinkedList<T>::end() {
        return iterator();
    }

    template<typename T>
    typename LinkedList<T>::const_iterator LinkedList<T>::begin() const {
        return const_iterator(head_);
    }

    template<typename T>
    typename LinkedList<T>::const_iterator LinkedList<T>::end() const {
        return const_iterator();
    }

}

#endif //SPIDER2_LINKEDLIST_H
