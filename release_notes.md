Spider2 Changelog
================

## Release version 6.6.6
*XXXX.XX.XX*

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
