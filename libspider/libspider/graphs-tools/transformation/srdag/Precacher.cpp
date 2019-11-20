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

/* === Include(s) === */

#include <graphs-tools/transformation/srdag/Precacher.h>
#include <common/EnumIterator.h>

/* === Static function === */

spider::srdag::AbstractPool *makePool(spider::pisdf::VertexType type, std::int32_t size) {
    using spider::srdag::Pool;
    switch (type) {
        case spider::pisdf::VertexType::NORMAL: {
            auto *pool = spider::allocate<Pool<PiSDFVertex>>(StackID::TRANSFO);
            spider::construct(pool, size);
            return pool;
        }
        case spider::pisdf::VertexType::CONFIG: {
            auto *pool = spider::allocate<Pool<PiSDFCFGVertex>>(StackID::TRANSFO);
            spider::construct(pool, size);
            return pool;
        }
        case spider::pisdf::VertexType::DELAY:
            return nullptr;
        case spider::pisdf::VertexType::FORK: {
            auto *pool = spider::allocate<Pool<PiSDFForkVertex>>(StackID::TRANSFO);
            spider::construct(pool, size);
            return pool;
        }
        case spider::pisdf::VertexType::JOIN: {
            auto *pool = spider::allocate<Pool<PiSDFJoinVertex>>(StackID::TRANSFO);
            spider::construct(pool, size);
            return pool;
        }
        case spider::pisdf::VertexType::REPEAT: {
            auto *pool = spider::allocate<Pool<PiSDFRepeatVertex>>(StackID::TRANSFO);
            spider::construct(pool, size);
            return pool;
        }
        case spider::pisdf::VertexType::DUPLICATE: {
            auto *pool = spider::allocate<Pool<PiSDFDuplicateVertex>>(StackID::TRANSFO);
            spider::construct(pool, size);
            return pool;
        }
        case spider::pisdf::VertexType::TAIL: {
            auto *pool = spider::allocate<Pool<PiSDFTailVertex>>(StackID::TRANSFO);
            spider::construct(pool, size);
            return pool;
        }
        case spider::pisdf::VertexType::HEAD: {
            auto *pool = spider::allocate<spider::srdag::Pool<PiSDFHeadVertex>>(StackID::TRANSFO);
            spider::construct(pool, size);
            return pool;
        }
        case spider::pisdf::VertexType::INIT: {
            auto *pool = spider::allocate<spider::srdag::Pool<PiSDFInitVertex>>(StackID::TRANSFO);
            spider::construct(pool, size);
            return pool;
        }
        case spider::pisdf::VertexType::END: {
            auto *pool = spider::allocate<spider::srdag::Pool<PiSDFEndVertex>>(StackID::TRANSFO);
            spider::construct(pool, size);
            return pool;
        }
        default:
            throwSpiderException("unsupported pool type: %d", static_cast<std::int32_t>(type));
    }
    return nullptr;
}

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::srdag::Precacher::Precacher(const spider::pisdf::Graph *graph, spider::pisdf::Graph *srdag) {
    if (!graph) {
        return;
    }
    std::array<std::int32_t, pisdf::VERTEX_TYPE_COUNT> typeCountArray = { 0 };
    /* == Count vertex for every type == */
    for (const auto &vertex : graph->vertices()) {
        typeCountArray[static_cast<std::int32_t>(vertex->subtype())] += vertex->repetitionValue();
    }

    /* == Add Tail and Repeat count based on interfaces == */
    typeCountArray[static_cast<std::int32_t>(pisdf::VertexType::NORMAL)] += typeCountArray[static_cast<std::int32_t>(pisdf::VertexType::GRAPH)];
    typeCountArray[static_cast<std::int32_t>(pisdf::VertexType::GRAPH)] = 0;
    typeCountArray[static_cast<std::int32_t>(pisdf::VertexType::REPEAT)] += graph->inputEdgeCount();
    typeCountArray[static_cast<std::int32_t>(pisdf::VertexType::TAIL)] += graph->outputEdgeCount();

    /* == Allocate array == */
    auto vertexCount = 0;
    for (auto type : spider::EnumIterator<pisdf::VertexType>()) {
        if ((type == pisdf::VertexType::INPUT) ||
            (type == pisdf::VertexType::OUTPUT)) {
            continue;
        }
        const auto &typeIx = static_cast<std::int32_t>(type);
        auto &&count = typeCountArray[typeIx];
        poolArray_[typeIx] = count ? makePool(type, count) : nullptr;
        vertexCount += (poolArray_[typeIx] ? count : 0);
    }

    /* == Reserve size in srdag == */
    srdag->precacheVertex(vertexCount);
}

spider::srdag::Precacher::Precacher(const spider::pisdf::Edge *edge, spider::pisdf::Graph *srdag) {

}

spider::srdag::Precacher::~Precacher() {
    for (auto &pool : poolArray_) {
        if (pool) {
            spider::destroy(pool);
            spider::deallocate(pool);
            pool = nullptr;
        }
    }
}


