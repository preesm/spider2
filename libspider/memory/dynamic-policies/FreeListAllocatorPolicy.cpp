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

/* === Includes === */

#include <memory/dynamic-policies/FreeListAllocatorPolicy.h>

/* === Constant(s) === */

size_t FreeListAllocatorPolicy::MIN_CHUNK_SIZE = 8192;

/* === Methods implementation === */

FreeListAllocatorPolicy::FreeListAllocatorPolicy(size_t staticBufferSize,
                                                 void *externalBuffer,
                                                 FreeListPolicy policy,
                                                 size_t alignment) :
        AbstractAllocatorPolicy(alignment) {
    if (alignment < 8) {
        throwSpiderException("Memory alignment should be at least of size sizeof(uint64_t) = 8 bytes.");
    }

    /* == We need extra space for the Node structure == */
    if (externalBuffer) {
        staticBufferPtr_ = externalBuffer;
        staticBufferSize_ = staticBufferSize;
        external_ = true;
    } else {
        staticBufferSize_ = std::max(staticBufferSize, MIN_CHUNK_SIZE);
        staticBufferPtr_ = std::malloc(staticBufferSize_ + sizeof(Node));
    }
    if (policy == FreeListPolicy::FIND_FIRST) {
        findNode_ = FreeListAllocatorPolicy::findFirst;
    } else if (policy == FreeListPolicy::FIND_BEST) {
        findNode_ = FreeListAllocatorPolicy::findBest;
    }
    /* == Reset the default == */
    list_ = reinterpret_cast<Node *>(staticBufferPtr_);
    list_->blockSize_ = staticBufferSize_;
    list_->next_ = nullptr;
}


FreeListAllocatorPolicy::~FreeListAllocatorPolicy() noexcept {
    if (!external_) {
        std::free(staticBufferPtr_);
    }
    for (auto &it: extraBuffers_) {
        std::free(it.bufferPtr_);
        it.bufferPtr_ = nullptr;
    }
}

std::pair<void *, size_t> FreeListAllocatorPolicy::allocate(size_t size) {
    if (!size) {
        return std::make_pair(nullptr, 0);
    }
    if (size < sizeof(Node)) {
        size = size + sizeof(Node);
    }
    size = size + sizeof(size_t);
    size_t padding = 0;

    /* == Find first / best node fitting memory requirement == */
    const auto &result = findNode_(size, &padding, alignment_, list_);
    auto *memoryNode = result.first;
    auto *baseNode = result.second;
    if (!memoryNode) {
        /* == Add extra buffer == */
        memoryNode = createExtraBuffer(size, baseNode);

        /* == Padding is ZERO == */
        padding = 0;
    }

    /* == Compute real required size == */
    auto requiredSize = size + padding;

    /* == Update the list of FreeNode == */
    updateFreeNodeList(baseNode, memoryNode, requiredSize);

    /* == Computing header and data address == */
    const auto &dataAddress = reinterpret_cast<uintptr_t>(memoryNode) + sizeof(size_t);

    /* == Write header info == */
    auto *header = reinterpret_cast<size_t *>(reinterpret_cast<void *>(memoryNode));
    (*header) = requiredSize;

    /* == Updating usage stats == */
    usage_ += requiredSize;
    return std::make_pair(reinterpret_cast<void *>(dataAddress), requiredSize);
}

void FreeListAllocatorPolicy::updateFreeNodeList(FreeListAllocatorPolicy::Node *baseNode,
                                                 FreeListAllocatorPolicy::Node *memoryNode,
                                                 size_t requiredSize) {
    const auto &leftOverMemory = memoryNode->blockSize_ - requiredSize;
    if (leftOverMemory) {
        /* == We split block to limit waste memory space == */
        auto *freeNode = reinterpret_cast<Node *>(reinterpret_cast<uintptr_t>(memoryNode) + requiredSize);
        freeNode->blockSize_ = leftOverMemory;
        insert(memoryNode, freeNode);
    }
    remove(baseNode, memoryNode);
}

size_t FreeListAllocatorPolicy::deallocate(void *ptr) {
    if (!ptr) {
        return 0;
    } else if (!usage_) {
        throwSpiderException("bad memory free: no memory allocated.");
    }

    /* == Read header info == */
    const auto &originalBufferAddress = reinterpret_cast<uintptr_t>(ptr) - sizeof(size_t);
    const auto &size = reinterpret_cast<size_t *>(originalBufferAddress)[0];
    auto *freeNode = reinterpret_cast<Node *>(originalBufferAddress);

    /* == Check address == */
    if (!validAddress(freeNode)) {
        throwSpiderException("bad memory free: memory address out of allocated space.");
    }
    freeNode->blockSize_ = size;
    freeNode->next_ = nullptr;

    Node *it = list_;
    Node *itPrev = nullptr;
    while (it) {
        if (reinterpret_cast<uintptr_t>(freeNode) < reinterpret_cast<uintptr_t>(it)) {
            insert(itPrev, freeNode);
            break;
        }
        itPrev = it;
        it = it->next_;
    }

    /* == Update internal usage == */
    usage_ -= freeNode->blockSize_;

    /* == Look for contiguous block to merge (coalescence) == */
    if (freeNode->next_ && (reinterpret_cast<uintptr_t>(freeNode) + freeNode->blockSize_ ==
                            reinterpret_cast<uintptr_t>(freeNode->next_))) {
        freeNode->blockSize_ += freeNode->next_->blockSize_;
        remove(freeNode, freeNode->next_);
    }
    if (itPrev && (reinterpret_cast<uintptr_t>(itPrev) + itPrev->blockSize_ ==
                   reinterpret_cast<uintptr_t>(freeNode))) {
        itPrev->blockSize_ += freeNode->blockSize_;
        remove(itPrev, freeNode);
    }
    return size;
}

void FreeListAllocatorPolicy::insert(Node *baseNode, Node *newNode) {
    if (!baseNode) {
        /* == Insert node as first == */
        newNode->next_ = list_;
        list_ = newNode;
    } else {
        /* == Insert node as last if baseNode->next == nullptr == */
        /* == Insert node in middle else == */
        newNode->next_ = baseNode->next_;
        baseNode->next_ = newNode;
    }
}

void FreeListAllocatorPolicy::remove(Node *baseNode, Node *removedNode) {
    if (!baseNode) {
        /* == Remove the first node == */
        list_ = removedNode->next_;
    } else {
        /* == Remove node in the list == */
        baseNode->next_ = removedNode->next_;
    }
}

FreeListAllocatorPolicy::Node *
FreeListAllocatorPolicy::createExtraBuffer(size_t size, FreeListAllocatorPolicy::Node *base) {
    /* == Allocate new buffer with size aligned to MIN_CHUNK == */
    FreeListAllocatorPolicy::Buffer buffer;
    buffer.size_ = AbstractAllocatorPolicy::computeAlignedSize(size, MIN_CHUNK_SIZE * allocScale_);
    buffer.bufferPtr_ = std::malloc(buffer.size_ + sizeof(Node));

    /* == Initialize memoryNode == */
    auto *node = reinterpret_cast<Node *>(buffer.bufferPtr_);
    node->blockSize_ = buffer.size_ - sizeof(Node);
    node->next_ = nullptr;

    /* == Add the new node to the existing list of free node == */
    insert(base, node);

    /* == Push buffer into vector to keep track of it == */
    extraBuffers_.push_back(buffer);

    allocScale_ *= 2;
    return node;
}

std::pair<FreeListAllocatorPolicy::Node *, FreeListAllocatorPolicy::Node *>
FreeListAllocatorPolicy::findFirst(size_t size, size_t *padding, size_t alignment, Node *base) {
    (*padding) = AbstractAllocatorPolicy::computePadding(size, alignment);
    const auto &requiredSize = size + (*padding);
    Node *previousNode = nullptr;
    auto *freeNode = base;
    while (freeNode) {
        if (freeNode->blockSize_ >= requiredSize) {
            return std::make_pair(freeNode, previousNode);
        }
        previousNode = freeNode;
        freeNode = freeNode->next_;
    }
    return std::make_pair(nullptr, nullptr);
}

std::pair<FreeListAllocatorPolicy::Node *, FreeListAllocatorPolicy::Node *>
FreeListAllocatorPolicy::findBest(size_t size, size_t *padding, size_t alignment, Node *base) {
    (*padding) = AbstractAllocatorPolicy::computePadding(size, alignment);
    auto &&minFit = SIZE_MAX;
    const auto &requiredSize = size + (*padding);
    auto *it = base;
    Node *previousNode = nullptr;
    Node *bestPreviousNode = nullptr;
    Node *bestNode = nullptr;
    while (it) {
        if ((it->blockSize_ >= requiredSize) &&
            ((it->blockSize_ - requiredSize) < minFit)) {
            minFit = it->blockSize_ - requiredSize;
            bestPreviousNode = previousNode;
            bestNode = it;
            if (!minFit) {
                /* == We won't find better fit == */
                return std::make_pair(bestNode, bestPreviousNode);
            }
        }
        previousNode = it;
        it = it->next_;
    }
    return std::make_pair(bestNode, bestPreviousNode);
}

bool FreeListAllocatorPolicy::validAddress(void *ptr) noexcept {
    const auto &uintptr = reinterpret_cast<uintptr_t>(ptr);
    const auto &staticBufferUintptr = reinterpret_cast<uintptr_t>(staticBufferPtr_);
    auto found = ((uintptr >= staticBufferUintptr) && (uintptr < (staticBufferUintptr + staticBufferSize_)));
    if (!found) {
        for (auto &it: extraBuffers_) {
            const auto &bufferUintptr = reinterpret_cast<uintptr_t>(it.bufferPtr_);
            found = ((uintptr >= bufferUintptr) && (uintptr < (bufferUintptr + it.size_)));
            if (found) {
                return true;
            }
        }
        return false;
    }
    return true;
}

