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
#ifndef SPIDER2_FREELISTALLOCATORPOLICY_H
#define SPIDER2_FREELISTALLOCATORPOLICY_H

/* === Includes === */

#include <vector>
#include <memory/allocator/AbstractAllocatorPolicy.h>

/* === Enumeration(s) === */

enum class FreeListPolicy {
    FIND_FIRST = 0,
    FIND_BEST = 1
};

/* === Class definition === */

class FreeListAllocatorPolicy final : public AbstractAllocatorPolicy {
public:
    struct Node {
        size_t blockSize_ = 0;
        Node *next_ = nullptr;

    };

    explicit FreeListAllocatorPolicy(size_t staticBufferSize,
                                     void *externalBuffer = nullptr,
                                     FreeListPolicy policy = FreeListPolicy::FIND_FIRST,
                                     size_t alignment = sizeof(int64_t));

    ~FreeListAllocatorPolicy() noexcept override;

    void *allocate(size_t size) override;

    u64 deallocate(void *ptr) override;

    static size_t MIN_CHUNK_SIZE;
private:

    struct Buffer {
        size_t size_ = 0;
        void *bufferPtr_ = nullptr;
    };

    Node *list_ = nullptr;

    void *staticBufferPtr_ = nullptr;
    bool external_ = false;
    std::vector<Buffer> extraBuffers_;
    size_t staticBufferSize_ = 0;
    size_t allocScale_ = 1;


    using FreeListPolicyMethod = std::pair<Node *, Node *> (*)(size_t, size_t *, size_t, Node *);

    FreeListPolicyMethod findNode_;

    void insert(Node *baseNode, Node *newNode);

    void remove(Node *baseNode, Node *removedNode);

    Node *createExtraBuffer(size_t size, Node *base);

    void updateFreeNodeList(Node *baseNode, Node *memoryNode, size_t requiredSize);

    static std::pair<Node *, Node *>
    findFirst(size_t size, size_t *padding, size_t alignment, Node *baseNode);

    static std::pair<Node *, Node *>
    findBest(size_t size, size_t *padding, size_t alignment, Node *baseNode);

    /**
     * @brief Check the pointer address to be sure we are deallocating memory we allocated.
     * @param ptr  Pointer to check.
     * @return true if address is valid, false else.
     */
    bool validAddress(void *ptr) noexcept;
};

#endif //SPIDER2_FREELISTALLOCATORPOLICY_H
