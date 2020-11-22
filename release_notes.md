Spider2 Changelog
================

## Release version 1.0.2
*2020.11.22*

### New Feature

### Changes
* Drastic improvement in mapping performance in PiSDF mode.
* Drastic improvement in memory alloc perfs in PiSDF mode.
* Removed unnecessary abstraction for single rate transformation.

### Bug fix
* Fixed support of external interfaces in PiSDF mode.


## Release version 1.0.1
*2020.11.01*

### New Feature

### Changes

### Bug fix


## Release version 1.0.0
*2020.11.01*

### New Feature
* Adding new DEFAULT_NOSYNC allocator (which only work post scheduling) that removes synchronisation points on forks, duplicates and extern interfaces. Default remains DEFAULT allocator.
* Adding full support of SR-DAG less execution of both static and dynamic applications (see: https://hal.archives-ouvertes.fr/hal-02355636 for more details).
* Adding support of round-robin mapper.

### Changes
* Changed Semaphore.h to use POSIX semaphore on Linux.
* Changed how send and receive task work in an heterogeneous setup.
* Decoupled Mapper and Scheduler. 
* Start up time with JIT Compiled expressions is now order of magnitude faster (usage of 1 unique file with only 1 dynamic compilation for all expressions). 
* Adding support of new operators in expression parser (==, >, <, >=, <=, !=).

### Bug fix
* Sonar: fixed const qualified function parameters.
* Sonar: fixed const qualified variables.
* Sonar: fixed implicit string concatenation.
* Fixed various bugs linked to the dynamic behavior of applications.
* Fixed join/fork optimization pattern in some corner cases.

## Release version 0.1.1
*2020.06.19*

### New Feature

### Changes

### Bug fix
* Fixed support of inter-cluster communication (scheduling, runtime and memalloc).

## Release version 0.1.0
*2020.06.18*

### New Feature

### Changes
* Updated expression parser with just-in-time compilation on Linux and optimized runtime parsing on Windows.
* Removed c++11 custom typesafe printf (was not used).
* Updated parameter creation API (merged static and dynamic derived parameter creation into one api call).
* Removed abstract classes for graph internal API.

### Bug fix
* The SRDAGJITMSRuntime does no longer consider subgraph with inherited dynamic parameters as fully dynamic.
* Fixed handling of inherited parameters with different names.
* Fixed compile issues on Windows.
* Fixed join/fork optimization pattern in some corner cases.


## Release version 0.0.0
*2020.04.14*

### New Feature
* Implementation of a PiSDF runtime (see https://hal.inria.fr/hal-00877492).
* Based on the original SPIDER runtime (see https://github.com/preesm/spider).
* Expression parsing
    * Expression parser support basic math operations: +,-,*,/,^, integer modulo.
    * Expression parser support higher order math functions: factorial; trigonometric functions; exponential; logarithm (natural, base 2 and base 10); ceil/floor; absolute value; min/max.
    * Expression parser also support simple conditionnal statements: if-else constructs (using if(predicate,true,false)); logical AND/OR (using and/or(a,b)); < (using less), <= (using leq), > (using grt), >= (using geq);
    * Customized expression support can be added fairly easily. 
* Model support
    * Support all features of PiSDF Model of Computation **except** for dynamic delay expressions.
    * Support extended PiSDF MoC with State-Aware Meta-model semantic (see https://hal.archives-ouvertes.fr/hal-01850252/document).
    * Adding extra support for external interfaces to the graph: 
        -This allows the spider2 runtime to be used as an ad-hoc accelerator instead of an application manager. (i.e application does not need to be fully model with dataflow semantics, and data can be fed to and from the outside of the graph).
* Programming API
    * Support for automated code generation using the PREESM tool (see https://github.com/preesm/preesm)
    * Very comprehensive API to manipulate and tune the spider2 runtime.
* Debug support:
    * Support for execution traces retrieval with post and pre execution schedule gantt export.
    * Possibility to build and export graph in dot format fairly easily.
    * When using the JITMS runtime (see https://tel.archives-ouvertes.fr/tel-01301642), the single rate graph can be exported.
    * Support for tunnable verbosity level (ex: display only information about intermediate transformation).
    * Possibility to export gantt as SVG or XML.
    * Provides a sandbox main.cpp (which is kind of a mess right now) to show how to test an manipulate the library.
* Integration
    * Support for unit tests (not 100% coverage for now).

### Changes

### Bug fix
