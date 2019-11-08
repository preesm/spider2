/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
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

#include <chrono>
#include <iostream>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/params/InHeritedParam.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <spider.h>
#include <graphs-tools/brv/BRVCompute.h>
#include <graphs-tools/brv/LCMBRVCompute.h>
#include <graphs-tools/brv/TopologyBRVCompute.h>
#include <graphs-tools/exporter/DOTExporter.h>
#include <graphs-tools/transformation/SRDAGTransformation.h>
#include <runtime/master-slave/JITMSRuntime.h>
#include <containers/GenericSet.h>
#include <scheduling/schedule/Schedule.h>
#include <iomanip>
#include <scheduling/schedule/exporter/XMLGanttExporter.h>
#include <scheduling/schedule/exporter/SVGGanttExporter.h>
#include <common/Printer.h>
#include <memory>
#include <graphs-tools/expression-parser/RPNConverter.h>
#include <graphs-tools/expression-parser/Expression.h>

void createArchi();

void spiderTest();

class Base;

class MyIterator : public std::iterator<std::forward_iterator_tag, int> {
public:
    explicit MyIterator(std::vector<int>::iterator it) : it_{it} { }

    inline std::string operator*() {
        return std::to_string(*(it_));
    }

    inline std::string operator->() {
        return std::to_string(*(it_));
    }

    inline MyIterator operator++(int) {
        const auto &tmp = MyIterator(*this);
        (it_)++;
        return tmp;
    }

    inline MyIterator &operator++() {
        ++(it_);
        return *this;
    }

    inline bool operator==(const MyIterator &rhs) const {
        return it_ == rhs.it_;
    }

    inline bool operator!=(const MyIterator &rhs) const {
        return it_ != rhs.it_;
    }

private:
    std::vector<int>::iterator it_;
};

class IteratorTest {
public:
    IteratorTest() : it(values_.begin()) {

    }

    inline MyIterator begin() {
        return MyIterator(values_.begin());
    }

    inline MyIterator end() {
        return MyIterator(values_.end());
    }

    std::vector<int> values_;
    MyIterator it;
};

class Base {

};

class Derived : public Base {
public:
    std::vector<Derived *> values_;

};

template<class T>
void draw(const T &x, std::ostream &out, size_t position) {
    out << std::string(position, ' ') << x << std::endl;
}

class object_t {
public:
    template<class T>
    object_t(T x) : self_{new model<T>(std::move(x))} { }

    friend void draw(const object_t &x, std::ostream &out, size_t position) {
        x.self_->draw_(out, position);
    }

private:
    struct concept_t {
        virtual ~concept_t() = default;

        virtual concept_t *copy_() const = 0;

        virtual void draw_(std::ostream &, size_t) const = 0;
    };

    template<class T>
    struct model : concept_t {
        model(T x) : data_(std::move(x)) { }

        concept_t *copy_() const override { return new model(*this); }

        void draw_(std::ostream &out, size_t position) const override {
            draw(data_, out, position);
        }

        T data_;
    };

    std::shared_ptr<const concept_t> self_;
};

using document_t = std::vector<object_t>;

void draw(const document_t &x, std::ostream &out, size_t position) {
    out << std::string(position, ' ') << "<document>" << std::endl;
    for (const auto &e : x) {
        draw(e, out, position + 2);
    }
    out << std::string(position, ' ') << "</document>" << std::endl;
}

class my_class_t {

};

void draw(const my_class_t &, std::ostream &out, size_t position) {
    out << std::string(position, ' ') << "my_class_t" << std::endl;
}

int main(int, char **) {
    spiderTest();
//    std::cout << "sizeof(Spider::PiSDF::Vertex): " << sizeof(Spider::PiSDF::Vertex) << std::endl;
//    std::cout << "sizeof(Spider::PiSDF::ExecVertex): " << sizeof(Spider::PiSDF::ExecVertex) << std::endl;
//    std::cout << "sizeof(Spider::PiSDF::ForkVertex): " << sizeof(Spider::PiSDF::ForkVertex) << std::endl;
//    std::cout << "sizeof(Spider::PiSDF::Interface): " << sizeof(Spider::PiSDF::Interface) << std::endl;
//    std::cout << "sizeof(Spider::PiSDF::InputInterface): " << sizeof(Spider::PiSDF::InputInterface) << std::endl;
//    std::cout << "sizeof(Spider::PiSDF::Graph): " << sizeof(Spider::PiSDF::Graph) << std::endl;
//    std::cout << "sizeof(Spider::PiSDF::Edge): " << sizeof(Spider::PiSDF::Edge) << std::endl;
//    std::cout << "sizeof(Spider::PiSDF::Delay): " << sizeof(Spider::PiSDF::Delay) << std::endl;
//    std::cout << "sizeof(Spider::PiSDF::Param): " << sizeof(Spider::PiSDF::Param) << std::endl;
//    std::cout << "sizeof(Spider::PiSDF::InHeritedParam): " << sizeof(Spider::PiSDF::InHeritedParam) << std::endl;
//    std::cout << "sizeof(Spider::PiSDF::DynamicParam): " << sizeof(Spider::PiSDF::DynamicParam) << std::endl;
//    std::cout << "sizeof(Expression): " << sizeof(Expression) << std::endl;
//    std::cout << "sizeof(std::string): " << sizeof(std::string) << std::endl;
//    std::cout << "sizeof(Spider::vector<RPNElement>): " << sizeof(Spider::vector<RPNElement>) << std::endl;

    return 0;
}

void spiderTest() {
    Spider::API::start();
    Spider::API::initStack(StackID::PISDF, "pisdf-stack", AllocatorType::FREELIST, 16392, FreeListPolicy::FIND_FIRST);
    Spider::API::initStack(StackID::TRANSFO, "transfo-stack", AllocatorType::FREELIST, 16392,
                           FreeListPolicy::FIND_FIRST);
    Spider::API::initStack(StackID::EXPR_PARSER, "expr-parser-stack", AllocatorType::FREELIST, 4096,
                           FreeListPolicy::FIND_FIRST);
    Spider::API::initStack(StackID::ARCHI, "archi-stack", AllocatorType::FREELIST, 16392,
                           FreeListPolicy::FIND_FIRST);
    Spider::API::enableLogger(LOG_TRANSFO);
    Spider::API::enableLogger(LOG_OPTIMS);
    Spider::API::enableVerbose();

//    createArchi();

    {
        std::cerr << sizeof(Spider::Array<int>) << std::endl;
        std::cerr << sizeof(Spider::vector<int>) << std::endl;
    }

    for (std::uint32_t i = 0; i < 0; ++i) {
        auto *graph = Spider::API::createGraph("topgraph", 8, 7);

        /* === Creating vertices === */

        auto *vertex_0 = Spider::API::createVertex(graph, "vertex_0", 0, 1);
        auto *vertex_1 = Spider::API::createVertex(graph, "vertex_1", 1, 1);
        auto *subgraph = Spider::API::createSubraph(graph, "subgraph", 2, 3, 0, 1, 1);
        auto *input = Spider::API::setInputInterfaceName(subgraph, 0, "input");
        auto *output = Spider::API::setOutputInterfaceName(subgraph, 0, "output");
        auto *vertex_2 = Spider::API::createVertex(subgraph, "vertex_2", 1, 1);
        auto *vertex_3 = Spider::API::createVertex(subgraph, "vertex_3", 1, 1);
        auto *vertex_4 = Spider::API::createVertex(graph, "vertex_4", 1);

        /* === Creating param === */


        /* === Creating edges === */

        auto *edge = Spider::API::createEdge(vertex_0, 0, 5, vertex_1, 0, 5);
        Spider::API::createEdge(vertex_1, 0, 5, subgraph, 0, 5);
        Spider::API::createEdge(input, 0, 5, vertex_2, 0, 5);
        Spider::API::createEdge(vertex_2, 0, 1, vertex_3, 0, 5);
        Spider::API::createEdge(vertex_3, 0, 2, output, 0, 5);
        Spider::API::createEdge(subgraph, 0, 5, vertex_4, 0, 5);
        Spider::API::createDelay(edge, 6, nullptr, 0, 0, nullptr, 0, 0);

        {
            auto *srdag = Spider::API::createGraph("srdag", 10, 9, 0, 0, 0, 0, StackID::TRANSFO);
            auto job = Spider::SRDAG::Job(graph, 0, UINT32_MAX);
            job.params_ = graph->params();
            const auto &jobs = Spider::SRDAG::staticSingleRateTransformation(job, srdag);
            if (!jobs.first.empty()) {
                Spider::SRDAG::staticSingleRateTransformation(jobs.first.back(), srdag);
            }
            auto exporter = Spider::PiSDF::DOTExporter{srdag};
            exporter.print("./srdag.dot");
            Spider::destroy(srdag);
            Spider::deallocate(srdag);
        }

        /* === Export dot === */

//        auto exporter = Spider::PiSDF::DOTExporter{graph};
//        exporter.print("./new.dot");


//        {
//            using namespace Spider;
//            Schedule schedule;
//            auto job = ScheduleJob(0, 0, 0, 0, 0);
//            job.setMappingStartTime(15);
//            job.setMappingEndTime(115);
//            schedule.add(std::move(job));
//            job = ScheduleJob(1, 1, 0, 0, 0);
//            job.setMappingStartTime(120);
//            job.setMappingEndTime(320);
//            schedule.add(std::move(job));
//            job = ScheduleJob(2, 2, 1, 0, 0);
//            job.setMappingStartTime(60);
//            job.setMappingEndTime(80);
//            schedule.add(std::move(job));
//            job = ScheduleJob(3, 3, 2, 0, 0);
//            job.setMappingStartTime(0);
//            job.setMappingEndTime(700);
//            schedule.add(std::move(job));
//
//            pisdfGraph() = graph;
//
//            SVGGanttExporter ganttExporter{&schedule};
//            ganttExporter.print();
//        }

        Spider::destroy(graph);
        Spider::deallocate(graph);
    }
    Spider::API::quit();
}

void createArchi() {
    Spider::API::createPlatform();

    auto *x86MemoryUnit = Spider::API::createMemoryUnit(nullptr, 20000);

    auto *x86Cluster = Spider::API::createCluster(4, x86MemoryUnit);

    auto x86PECore0 = Spider::API::createPE(0, 0, 0,
                                            x86Cluster,
                                            "x86-Core0",
                                            Spider::PEType::LRT_PE,
                                            Spider::HWType::PHYS_PE);

    Spider::API::createPE(1, 1, 1,
                          x86Cluster,
                          "x86-Core1",
                          Spider::PEType::LRT_PE,
                          Spider::HWType::PHYS_PE);

    Spider::API::createPE(2, 2, 2,
                          x86Cluster,
                          "x86-Core2",
                          Spider::PEType::LRT_PE,
                          Spider::HWType::PHYS_PE);

    Spider::API::createPE(3, 3, 3,
                          x86Cluster,
                          "x86-Core3",
                          Spider::PEType::LRT_PE,
                          Spider::HWType::PHYS_PE);

    Spider::API::setSpiderGRTPE(x86PECore0);
}
