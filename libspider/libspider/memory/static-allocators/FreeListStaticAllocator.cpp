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

#include <common/memory/static-allocators/FreeListStaticAllocator.h>

/* === Methods implementation === */

FreeListStaticAllocator::FreeListStaticAllocator(std::string name,
                                                 std::uint64_t totalSize,
                                                 FreeListPolicy policy,
                                                 std::int32_t alignment) :
        StaticAllocator(std::move(name), totalSize + sizeof(FreeListStaticAllocator::Header), alignment) {
    if (alignment < 8) {
        throwSpiderException("Memory alignment should be at least of size sizeof(std::int64_t) = 8 bytes.");
    }
    this->reset();
    if (policy == FreeListPolicy::FIND_FIRST) {
        method_ = FreeListStaticAllocator::findFirst;
    } else if (policy == FreeListPolicy::FIND_BEST) {
        method_ = FreeListStaticAllocator::findBest;
    }
}

FreeListStaticAllocator::FreeListStaticAllocator(std::string name, std::uint64_t totalSize, char *externalBase,
                                                 FreeListPolicy policy, int32_t alignment) :
        StaticAllocator(std::move(name), totalSize + sizeof(FreeListStaticAllocator::Header), externalBase, alignment) {
    if (alignment < 8) {
        throwSpiderException("Memory alignment should be at least of size sizeof(std::int64_t) = 8 bytes.");
    }
    this->reset();
    if (policy == FreeListPolicy::FIND_FIRST) {
        method_ = FreeListStaticAllocator::findFirst;
    } else if (policy == FreeListPolicy::FIND_BEST) {
        method_ = FreeListStaticAllocator::findBest;
    }
}

void *FreeListStaticAllocator::allocate(std::uint64_t size) {
    if (!size) {
        return nullptr;
    }
    if ((std::size_t) size < sizeof(Node)) {
        size += sizeof(Node);
    }
    std::int32_t padding = 0;
    Node *baseNode = list_;
    Node *memoryNode = nullptr;
    /* == Find first / best node fitting memory requirement == */
    method_(size, padding, alignment_, baseNode, memoryNode);
    std::int32_t paddingWithoutHeader = padding - sizeof(FreeListStaticAllocator::Header);
    std::uint64_t requiredSize = size + padding;
    std::uint64_t leftOverMemory = memoryNode->blockSize_ - requiredSize;
    if (leftOverMemory) {
        /* == We split block to limit waste memory space == */
        auto *freeNode = (Node *) (((char *) memoryNode) + requiredSize);
        freeNode->blockSize_ = leftOverMemory;
        insert(memoryNode, freeNode);
    }
    remove(baseNode, memoryNode);
    /* == Computing header and data address == */
    char *headerAddress = (char *) (memoryNode) + paddingWithoutHeader;
    char *dataAddress = (char *) (memoryNode) + padding;

    /* == Write header info == */
    auto *header = (Header *) (headerAddress);
    header->size_ = requiredSize;
    header->padding_ = static_cast<uint64_t>(paddingWithoutHeader);

    /* == Updating usage stats == */
    used_ += requiredSize;
    peak_ = std::max(peak_, used_);
    return dataAddress;
}

void FreeListStaticAllocator::deallocate(void *ptr) {
    if (!ptr) {
        return;
    }
    auto *currentAddress = static_cast<char *>(ptr);
    char *headerAddress = currentAddress - sizeof(FreeListStaticAllocator::Header);

    /* == Read header info == */
    auto *header = (Header *) (headerAddress);
    auto *freeNode = (Node *) (headerAddress - header->padding_);
    /* == Check address == */
    StaticAllocator::checkPointerAddress(freeNode);
    freeNode->blockSize_ = header->size_;
    freeNode->next_ = nullptr;

    /* == Case where we allocated all memory at once == */
    if (!list_) {
        list_ = freeNode;
    }

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
    if (freeNode->next_ && ((char *) freeNode + freeNode->blockSize_) == ((char *) freeNode->next_)) {
        freeNode->blockSize_ += freeNode->next_->blockSize_;
        remove(freeNode, freeNode->next_);
    }
    if (itPrev && ((char *) itPrev + itPrev->blockSize_) == ((char *) freeNode)) {
        itPrev->blockSize_ += freeNode->blockSize_;
        remove(itPrev, freeNode);
    }

}

void FreeListStaticAllocator::reset() {
    averageUse_ += used_;
    numberAverage_++;
    used_ = 0;
    list_ = (Node *) (startPtr_);
    list_->blockSize_ = totalSize_;
    list_->next_ = nullptr;
}

void FreeListStaticAllocator::insert(Node *baseNode, Node *newNode) {
    if (!baseNode) {
        /* == Insert node as first == */
        newNode->next_ = list_ ? list_ : nullptr;
        list_ = newNode;
    } else {
        /* == Insert node as last if baseNode->next == nullptr == */
        /* == Insert node in middle else == */
        newNode->next_ = baseNode->next_;
        baseNode->next_ = newNode;
    }
}

void FreeListStaticAllocator::remove(Node *baseNode, Node *removedNode) {
    if (!baseNode) {
        /* == Remove the first node == */
        list_ = removedNode->next_;
    } else {
        /* == Remove node in the list == */
        baseNode->next_ = removedNode->next_;
    }
}

void
FreeListStaticAllocator::findFirst(std::uint64_t &size, std::int32_t &padding, std::int32_t &alignment,
                                   Node *&baseNode,
                                   Node *&foundNode) {
    Node *it = baseNode;
    baseNode = nullptr;
    constexpr std::int32_t headerSize = sizeof(FreeListStaticAllocator::Header);
    auto sizeWithHeader = size + headerSize;
    while (it) {
        if (it->blockSize_ >= sizeWithHeader) {
            padding = AbstractAllocator::computePadding(sizeWithHeader, alignment);
            padding += headerSize;
            std::uint64_t requiredSize = size + padding;
            if (it->blockSize_ >= requiredSize) {
                foundNode = it;
                return;
            }
        }
        baseNode = it;
        it = it->next_;
    }
    throwSpiderException("Not enough memory available for requested size of %"
                                 PRIu64
                                 "", size);
}

void
FreeListStaticAllocator::findBest(std::uint64_t &size, std::int32_t &padding, std::int32_t &alignment,
                                  Node *&baseNode,
                                  Node *&foundNode) {
    Node *head = baseNode;
    Node *it = head;
    baseNode = nullptr;
    std::uint64_t minFit = UINT64_MAX;
    constexpr std::int32_t headerSize = sizeof(FreeListStaticAllocator::Header);
    auto sizeWithHeader = size + headerSize;
    while (it) {
        if (it->blockSize_ >= sizeWithHeader) {
            padding = AbstractAllocator::computePadding(sizeWithHeader, alignment);
            padding += headerSize;
            std::uint64_t requiredSize = size + padding;
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
        }
        it = it->next_;
    }
    if (baseNode == head && head->next_ == nullptr) {
        baseNode = nullptr;
    }
    if (!foundNode) {
        throwSpiderException("Not enough memory available for requested size of %"
                                     PRIu64
                                     "", size);
    }
}
