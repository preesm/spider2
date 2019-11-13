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

#include <memory/dynamic-allocators/FreeListAllocator.h>

/* === Constant(s) === */

constexpr auto min_chunk_size = 8192;

/* === Methods implementation === */

FreeListAllocator::FreeListAllocator(std::string name,
                                     std::uint64_t staticBufferSize,
                                     FreeListPolicy policy,
                                     std::int32_t alignment) :
        DynamicAllocator(std::move(name), alignment),
        staticBufferSize_{ std::max(staticBufferSize, static_cast<std::uint64_t >(min_chunk_size)) } {
    if (alignment < 8) {
        throwSpiderException("Memory alignment should be at least of size sizeof(std::uint64_t) = 8 bytes.");
    }

    /* == We need extra space for the Node structure == */
    staticBufferPtr_ = std::malloc(staticBufferSize_ + sizeof(Node));
    this->reset();
    if (policy == FreeListPolicy::FIND_FIRST) {
        findNode_ = FreeListAllocator::findFirst;
    } else if (policy == FreeListPolicy::FIND_BEST) {
        findNode_ = FreeListAllocator::findBest;
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
    if (size < sizeof(Node)) {
        size = size + sizeof(Node);
    }
    std::int32_t padding = 0;

    /* == Find first / best node fitting memory requirement == */
    const auto &result = findNode_(size, padding, alignment_, list_);
    auto *memoryNode = result.first;
    auto *baseNode = result.second;
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
    const auto &paddingWithoutHeader = padding - sizeof(FreeListAllocator::Header);
    const auto &headerAddress = reinterpret_cast<std::uintptr_t>(memoryNode) + paddingWithoutHeader;
    const auto &dataAddress = reinterpret_cast<std::uintptr_t>(memoryNode) + padding;

    /* == Write header info == */
    auto *header = reinterpret_cast<Header *>(headerAddress);
    header->size_ = requiredSize;
    header->padding_ = static_cast<uint64_t>(paddingWithoutHeader);

    /* == Updating usage stats == */
    used_ += requiredSize;
    peak_ = std::max(peak_, used_);
    return reinterpret_cast<void *>(dataAddress);
}

void FreeListAllocator::updateFreeNodeList(FreeListAllocator::Node *baseNode,
                                           FreeListAllocator::Node *memoryNode,
                                           std::uint64_t requiredSize) {
    uint64_t leftOverMemory = memoryNode->blockSize_ - requiredSize;
    if (leftOverMemory) {
        /* == We split block to limit waste memory space == */
        auto *freeNode = reinterpret_cast<Node *>(reinterpret_cast<std::uintptr_t>(memoryNode) + requiredSize);
        freeNode->blockSize_ = leftOverMemory;
        insert(memoryNode, freeNode);
    }
    remove(baseNode, memoryNode);
}

void FreeListAllocator::deallocate(void *ptr) {
    if (!ptr) {
        return;
    } else if (!used_) {
        throwSpiderException("bad memory free: no memory allocated.");
    }

    /* == Read header info == */
    const auto &headerAddress = reinterpret_cast<std::uintptr_t>(ptr) - sizeof(FreeListAllocator::Header);
    auto *header = reinterpret_cast<Header *>(headerAddress);
    auto *freeNode = reinterpret_cast<Node *>(headerAddress - header->padding_);

    /* == Check address == */
    if (!validAddress(freeNode)) {
        throwSpiderException("bad memory free: memory address out of allocated space.");
    }
    freeNode->blockSize_ = header->size_;
    freeNode->next_ = nullptr;

    Node *it = list_;
    Node *itPrev = nullptr;
    while (it) {
        if (reinterpret_cast<std::uintptr_t>(freeNode) < reinterpret_cast<std::uintptr_t>(it)) {
            insert(itPrev, freeNode);
            break;
        }
        itPrev = it;
        it = it->next_;
    }

    /* == Update internal usage == */
    used_ -= freeNode->blockSize_;

    /* == Look for contiguous block to merge (coalescence) == */
    if (freeNode->next_ && (reinterpret_cast<std::uintptr_t>(freeNode) + freeNode->blockSize_ ==
                            reinterpret_cast<std::uintptr_t>(freeNode->next_))) {
        freeNode->blockSize_ += freeNode->next_->blockSize_;
        remove(freeNode, freeNode->next_);
    }
    if (itPrev && (reinterpret_cast<std::uintptr_t>(itPrev) + itPrev->blockSize_ ==
                   reinterpret_cast<std::uintptr_t>(freeNode))) {
        itPrev->blockSize_ += freeNode->blockSize_;
        remove(itPrev, freeNode);
    }
}

void FreeListAllocator::reset() {
    averageUse_ += used_;
    numberAverage_++;
    used_ = 0;
    list_ = reinterpret_cast<Node *>(staticBufferPtr_);
    list_->blockSize_ = staticBufferSize_;
    list_->next_ = nullptr;
    Node *currentNode = list_;
    for (auto &it: extraBuffers_) {
        auto *bufferHead = reinterpret_cast<Node *>(it.bufferPtr_);
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
    /* == Allocate new buffer with size aligned to MIN_CHUNK == */
    const auto &sizeWithHeader = size + sizeof(FreeListAllocator::Header);
    FreeListAllocator::Buffer buffer;
    buffer.size_ = AbstractAllocator::computeAlignedSize(sizeWithHeader, min_chunk_size);
    buffer.bufferPtr_ = std::malloc(buffer.size_ + sizeof(Node));

    /* == Initialize memoryNode == */
    auto *node = reinterpret_cast<Node *>(buffer.bufferPtr_);
    node->blockSize_ = buffer.size_ - sizeof(Node);
    node->next_ = nullptr;

    /* == Add the new node to the existing list of free node == */
    insert(base, node);

    /* == Push buffer into vector to keep track of it == */
    extraBuffers_.push_back(buffer);
    return node;
}

std::pair<FreeListAllocator::Node *, FreeListAllocator::Node *>
FreeListAllocator::findFirst(const std::uint64_t &size,
                             std::int32_t &padding,
                             const std::uint64_t &alignment,
                             Node *base) {
    padding = AbstractAllocator::computePaddingWithHeader(size, alignment, sizeof(FreeListAllocator::Header));
    const auto &requiredSize = size + padding;
    Node *previousNode = nullptr;
    auto *freeNode = base;
    while (freeNode) {
        if (freeNode->blockSize_ - sizeof(Node) >= requiredSize) {
            return std::make_pair(freeNode, previousNode);
        }
        previousNode = freeNode;
        freeNode = freeNode->next_;
    }
    return std::make_pair(nullptr, nullptr);
}

std::pair<FreeListAllocator::Node *, FreeListAllocator::Node *>
FreeListAllocator::findBest(const std::uint64_t &size,
                            std::int32_t &padding,
                            const std::uint64_t &alignment,
                            Node *base) {
    padding = AbstractAllocator::computePaddingWithHeader(size, alignment, sizeof(FreeListAllocator::Header));
    auto &&minFit = UINT64_MAX;
    const auto &requiredSize = size + padding;
    auto *it = base;
    Node *previousNode = nullptr;
    Node *bestNode = nullptr;
    while (it) {
        if ((it->blockSize_ - sizeof(Node) >= requiredSize) &&
            ((it->blockSize_ - requiredSize) < minFit)) {
            minFit = it->blockSize_ - requiredSize;
            if (!minFit) {
                /* == We won't find better fit == */
                return std::make_pair(it, previousNode);
            }
            bestNode = it;
        } else {
            previousNode = it;
        }
        it = it->next_;
    }
    if (previousNode == base && !base->next_) {
        return std::make_pair(bestNode, nullptr);
    }
    return std::make_pair(bestNode, previousNode);
}

bool FreeListAllocator::validAddress(void *ptr) {
    const auto &uintptr = reinterpret_cast<std::uintptr_t>(ptr);
    const auto &staticBufferUintptr = reinterpret_cast<std::uintptr_t>(staticBufferPtr_);
    auto found = ((uintptr >= staticBufferUintptr) && (uintptr < (staticBufferUintptr + staticBufferSize_)));
    if (!found) {
        for (auto &it: extraBuffers_) {
            const auto &bufferUintptr = reinterpret_cast<std::uintptr_t>(it.bufferPtr_);
            found = ((uintptr >= bufferUintptr) && (uintptr < (bufferUintptr + it.size_)));
            if (found) {
                return true;
            }
        }
        return false;
    }
    return true;
}

