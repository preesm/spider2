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
#ifndef SPIDER2_PISDFPARAM_H
#define SPIDER2_PISDFPARAM_H

/* === Includes === */

#include <cstdint>
#include <graphs-tools/expression-parser/Expression.h>
#include "PiSDFTypes.h"

/* === Type declarations === */

using Param = std::int64_t;

/* === Forward declaration(s) === */

class PiSDFGraph;

class PiSDFVertex;

/* === Class definition === */

class PiSDFParam {
public:

    PiSDFParam(PiSDFGraph *graph,
               std::string name,
               const std::string &expression);

    PiSDFParam(PiSDFGraph *graph,
               std::string name,
               std::int64_t value);

    PiSDFParam(PiSDFGraph *graph,
               std::string name,
               const PiSDFParam *parent);

    PiSDFParam(PiSDFGraph *graph,
               std::string name,
               PiSDFVertex *setter);

    ~PiSDFParam() = default;

    /* === Methods === */

    /**
     * @brief Get the dynamic property of the parameter.
     * @return true if parameter is dynamic, false else.
     */
    inline bool isDynamic() const;

    /**
     * @brief Export edge in the dot format to the given file.
     * @param file   File to which the edge should be exported.
     * @param offset Tab offset in the file.
     */
    void exportDot(FILE *file, const std::string &offset = "\t") const;

    /* === Setters === */

    /**
     * @brief Set the value of a dynamic parameter.
     * @param value value to set.
     * @throw @refitem SpiderException if the parameter is not of type @refitem PiSDFParamType::DYNAMIC.
     */
    inline void setValue(std::int64_t value);

    /**
     * @brief Set the ix of the param in the containing graph.
     * @param ix Ix to set.
     */
    inline void setIx(std::uint32_t ix);

    /* === Getters === */

    /**
   * @brief Get the containing @refitem PiSDFGraph of the parameter.
   * @return containing @refitem PiSDFGraph
   */
    inline PiSDFGraph *containingGraph() const;

    /**
     * @brief Get the string of the name of the parameter.
     * @return name of the parameter
     */
    inline const std::string &name() const;

    /**
     * @brief Get the vertex setting the value of the parameter (if any).
     * @return Vertex setting the parameter, nullptr else.
     */
    inline PiSDFVertex *setter() const;

    /**
     * @brief Get the value of the parameter (evaluate the Expression if needed).
     * @remark For dynamic parameters, it is up to the user to evaluate expression after parameter resolution.
     * @return Value of the parameter.
     */
    inline Param value() const;

    /**
     * @brief Get the @refitem PiSDFParamType of the parameter.
     * @return type of the parameter.
     */
    inline PiSDFParamType type() const;

    /**
     * @brief Get the ix of the edge in the containing graph.
     * @return ix of the edge (UINT32_MAX if no ix).
     */
    inline std::uint32_t ix() const;

private:
    /**
     * @brief Containing graph of the parameter.
     */
    PiSDFGraph *graph_ = nullptr;

    /**
     * @brief Name of the parameter within its containing graph.
     */
    std::string name_ = "unnamed-parameter";

    /**
     * @brief Parameter Type (STATIC, DYNAMIC, HERITED).
     */
    PiSDFParamType type_ = PiSDFParamType::STATIC;

    /**
     * @brief Vertex setting the parameter's value if it is of type DYNAMIC.
     */
    PiSDFVertex *setter_ = nullptr;

    /**
    * @brief Pointer to original parameter if parameter if of type HERITED.
    */
    const PiSDFParam *inheritedParam_ = nullptr;

    /**
     * @brief Expression of the parameter
     */
    Expression expression_;

    /**
     * @brief Ix of the parameter (unique per graph).
     */
    std::uint32_t ix_ = UINT32_MAX;
};

/* === Inline Methods === */

bool PiSDFParam::isDynamic() const {
    return type_ == PiSDFParamType::DYNAMIC || type_ == PiSDFParamType::DYNAMIC_DEPENDENT;
}

void PiSDFParam::setValue(std::int64_t value) {
    if (type_ != PiSDFParamType::DYNAMIC) {
        throwSpiderException("Trying to set value of a non dynamic parameter.");
    }
    expression_ = Expression(value);
}

void PiSDFParam::setIx(std::uint32_t ix) {
    ix_ = ix;
}

PiSDFGraph *PiSDFParam::containingGraph() const {
    return graph_;
}

const std::string &PiSDFParam::name() const {
    return name_;
}

PiSDFVertex *PiSDFParam::setter() const {
    return setter_;
}

Param PiSDFParam::value() const {
    if (inheritedParam_) {
        return inheritedParam_->value();
    }
    return expression_.evaluate();
}

PiSDFParamType PiSDFParam::type() const {
    return type_;
}

std::uint32_t PiSDFParam::ix() const {
    return ix_;
}

#endif //SPIDER2_PISDFPARAM_H
