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

/* === Includes === */

#include <memory/dynamic-allocators/FreeListAllocator.h>

/* === Methods implementation === */

FreeListAllocator::FreeListAllocator(std::string name,
                                     std::uint64_t staticBufferSize,
                                     FreeListPolicy policy,
                                     std::int32_t alignment) :
        DynamicAllocator(std::move(name), alignment),
        staticBufferSize_{std::max(staticBufferSize, static_cast<std::uint64_t >(MIN_CHUNK_SIZE))} {
    if (alignment < 8) {
        throwSpiderException("Memory alignment should be at least of size sizeof(std::int64_t) = 8 bytes.");
    }

    /* == We need extra space for the Node structure == */
    staticBufferPtr_ = static_cast<std::uint8_t *>(std::malloc((staticBufferSize_ + sizeof(Node)) * sizeof(char)));
    this->reset();
    if (policy == FreeListPolicy::FIND_FIRST) {
        method_ = FreeListAllocator::findFirst;
    } else if (policy == FreeListPolicy::FIND_BEST) {
        method_ = FreeListAllocator::findBest;
    }
}


FreeListAllocator::~FreeListAllocator() {
    std::free(staticBufferPtr_);
    for (auto &it: extraBuffers_) {
        std::free(it.bufferPtr_);
        it.bufferPtr_ = nullptr;
    }
}

void *FreeListAllocator::allocate(std::uint64_t size) {
    if (!size) {
        return nullptr;
    }
    if ((std::size_t) size < sizeof(Node)) {
        size = size + sizeof(Node);
    }
    std::int32_t padding = 0;
    Node *baseNode = list_;
    Node *memoryNode = nullptr;

    /* == Find first / best node fitting memory requirement == */
    method_(size, padding, alignment_, baseNode, memoryNode);
    if (!memoryNode) {
        /* == Add extra buffer == */
        memoryNode = createExtraBuffer(size, baseNode);

        /* == Padding is exactly the size of the Header == */
        padding = sizeof(FreeListAllocator::Header);
    }

    /* == Compute real required size == */
    auto requiredSize = size + padding;

    /* == Update the list of FreeNode == */
    updateFreeNodeList(baseNode, memoryNode, requiredSize);

    /* == Computing header and data address == */
    std::int32_t paddingWithoutHeader = padding - sizeof(FreeListAllocator::Header);
    auto *headerAddress = cast_buffer(memoryNode) + paddingWithoutHeader;
    auto *dataAddress = cast_buffer(memoryNode) + padding;

    /* == Write header info == */
    auto *header = (Header *) (headerAddress);
    header->size_ = requiredSize;
    header->padding_ = static_cast<uint64_t>(paddingWithoutHeader);

    /* == Updating usage stats == */
    used_ += requiredSize;
    peak_ = std::max(peak_, used_);
    return dataAddress;
}

void FreeListAllocator::updateFreeNodeList(FreeListAllocator::Node *baseNode,
                                           FreeListAllocator::Node *memoryNode,
                                           std::uint64_t requiredSize) {
    uint64_t leftOverMemory = memoryNode->blockSize_ - requiredSize;
    if (leftOverMemory) {
        /* == We split block to limit waste memory space == */
        Node *freeNode = cast_node(cast_buffer(memoryNode) + requiredSize);
        freeNode->blockSize_ = leftOverMemory;
        insert(memoryNode, freeNode);
    }
    remove(baseNode, memoryNode);
}

void FreeListAllocator::deallocate(void *ptr) {
    if (!ptr) {
        return;
    }
    auto *currentAddress = cast_buffer(ptr);
    auto *headerAddress = currentAddress - sizeof(FreeListAllocator::Header);

    /* == Read header info == */
    auto *header = (Header *) (headerAddress);
    Node *freeNode = cast_node(headerAddress - header->padding_);
    /* == Check address == */
    checkPointerAddress(freeNode);
    freeNode->blockSize_ = header->size_;
    freeNode->next_ = nullptr;

    Node *it = list_;
    Node *itPrev = nullptr;
    while (it) {
        if (ptr < it) {
            insert(itPrev, freeNode);
            break;
        }
        itPrev = it;
        it = it->next_;
    }

    /* == Update internal usage == */
    used_ -= freeNode->blockSize_;

    /* == Look for contiguous block to merge (coalescence) == */
    if (freeNode->next_ && (cast_buffer(freeNode) + freeNode->blockSize_) == (cast_buffer(freeNode->next_))) {
        freeNode->blockSize_ += freeNode->next_->blockSize_;
        remove(freeNode, freeNode->next_);
    }
    if (itPrev && (cast_buffer(itPrev) + itPrev->blockSize_) == (cast_buffer(freeNode))) {
        itPrev->blockSize_ += freeNode->blockSize_;
        remove(itPrev, freeNode);
    }
}

void FreeListAllocator::reset() {
    averageUse_ += used_;
    numberAverage_++;
    used_ = 0;
    list_ = cast_node(staticBufferPtr_);
    list_->blockSize_ = staticBufferSize_;
    list_->next_ = nullptr;
    Node *currentNode = list_;
    for (auto &it: extraBuffers_) {
        auto *bufferHead = cast_node(it.bufferPtr_);
        bufferHead->blockSize_ = it.size_;
        bufferHead->next_ = nullptr;
        insert(currentNode, bufferHead);
        currentNode = bufferHead;
    }
}

void FreeListAllocator::insert(Node *baseNode, Node *newNode) {
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

void FreeListAllocator::remove(Node *baseNode, Node *removedNode) {
    if (!baseNode) {
        /* == Remove the first node == */
        list_ = removedNode->next_;
    } else {
        /* == Remove node in the list == */
        baseNode->next_ = removedNode->next_;
    }
}

FreeListAllocator::Node *FreeListAllocator::createExtraBuffer(std::uint64_t size, FreeListAllocator::Node *base) {
    /* == We need to allocate new chunks of memory == */
    auto padding = sizeof(FreeListAllocator::Header);

    /* == Allocate new buffer with size aligned to MIN_CHUNK == */
    auto sizeWithHeader = size + padding;
    FreeListAllocator::Buffer buffer;
    buffer.size_ = AbstractAllocator::computeAlignedSize(sizeWithHeader, MIN_CHUNK_SIZE);
    buffer.bufferPtr_ = cast_buffer(std::malloc((buffer.size_ + sizeof(Node)) * sizeof(char)));

    /* == Initialize memoryNode == */
    auto *node = cast_node(buffer.bufferPtr_);
    node->blockSize_ = buffer.size_ - sizeof(Node);
    node->next_ = nullptr;

    /* == Add the new node to the existing list of free node == */
    insert(base, node);

    /* == Push buffer into vector to keep track of it == */
    extraBuffers_.push_back(buffer);
    return node;
}

void FreeListAllocator::findFirst(std::uint64_t &size,
                                  std::int32_t &padding,
                                  std::int32_t &alignment,
                                  Node *&baseNode,
                                  Node *&foundNode) {
    Node *it = baseNode;
    baseNode = nullptr;
    constexpr std::int32_t headerSize = sizeof(FreeListAllocator::Header);
    padding = AbstractAllocator::computePaddingWithHeader(size, alignment, headerSize);
    auto requiredSize = size + padding;
    while (it) {
        if (it->blockSize_ >= requiredSize) {
            foundNode = it;
            return;
        }
        baseNode = it;
        it = it->next_;
    }
}

void FreeListAllocator::findBest(std::uint64_t &size,
                                 std::int32_t &padding,
                                 std::int32_t &alignment,
                                 Node *&baseNode,
                                 Node *&foundNode) {
    Node *head = baseNode;
    Node *it = head;
    baseNode = nullptr;
    std::uint64_t minFit = UINT64_MAX;
    constexpr std::int32_t headerSize = sizeof(FreeListAllocator::Header);
    padding = AbstractAllocator::computePaddingWithHeader(size, alignment, headerSize);
    auto requiredSize = size + padding;
    while (it) {
        if (it->blockSize_ >= requiredSize && ((it->blockSize_ - requiredSize) < minFit)) {
            foundNode = it;
            minFit = it->blockSize_ - requiredSize;
            if (minFit == 0) {
                /* == We won't find better fit == */
                return;
            }
        } else {
            baseNode = it;
        }
        it = it->next_;
    }
    if (baseNode == head && head->next_ == nullptr) {
        baseNode = nullptr;
    }
}

void FreeListAllocator::checkPointerAddress(void *ptr) {
    if (used_ == 0) {
        throwSpiderException("Trying to deallocate unallocated memory block.");
    }
    if (cast_buffer(ptr) < staticBufferPtr_ ||
        cast_buffer(ptr) > (staticBufferPtr_ + staticBufferSize_)) {
        for (auto &it: extraBuffers_) {
            if (cast_buffer(ptr) >= it.bufferPtr_ &&
                cast_buffer(ptr) < (it.bufferPtr_ + it.size_)) {
                return;
            }
        }
        throwSpiderException("Trying to deallocate memory block out of memory space.");
    }
}

