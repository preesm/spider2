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
#ifndef SPIDER2_FREELISTSTATICALLOCATOR_H
#define SPIDER2_FREELISTSTATICALLOCATOR_H

/* === Includes === */

#include <memory/abstract-allocators/StaticAllocator.h>

/* === Class definition === */

class FreeListStaticAllocator final : public StaticAllocator {
public:

    explicit FreeListStaticAllocator(std::string name,
                                     std::uint64_t totalSize,
                                     FreeListPolicy policy = FreeListPolicy::FIND_FIRST,
                                     std::uint64_t alignment = sizeof(std::int64_t));


    explicit FreeListStaticAllocator(std::string name,
                                     std::uint64_t totalSize,
                                     void *externalBase,
                                     FreeListPolicy policy = FreeListPolicy::FIND_FIRST,
                                     std::uint64_t alignment = sizeof(std::int64_t));

    ~FreeListStaticAllocator() override = default;

    void *allocate(std::uint64_t size) override;

    void deallocate(void *ptr) override;

    void reset() override;

    typedef struct Node {
        std::uint64_t blockSize_;
        Node *next_;
    } Node;

private:
    typedef struct Header {
        std::uint64_t size_;
        std::uint64_t padding_;
    } Header;

    Node *list_ = nullptr;

    void insert(Node *baseNode, Node *newNode);

    void remove(Node *baseNode, Node *removedNode);

    using policyMethod = void (*)(std::uint64_t &, std::uint64_t &, std::uint64_t &, Node *&, Node *&);

    policyMethod method_;

    static void
    findFirst(std::uint64_t &size, std::uint64_t &padding, std::uint64_t &alignment, Node *&baseNode, Node *&foundNode);

    static void
    findBest(std::uint64_t &size, std::uint64_t &padding, std::uint64_t &alignment, Node *&baseNode, Node *&foundNode);
};

#endif //SPIDER2_FREELISTSTATICALLOCATOR_H
