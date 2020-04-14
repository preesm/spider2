# <img src="./doc/img/spider-full-nofont.svg" alt="SPIDER 2.0" width="400"/>
_Synchronous Parameterized and Interfaced Dataflow Embedded Runtime_


SPIDER is an open-source Dataflow based runtime for running signal processing applications based on the [Parameterized and Interfaced Based Synchronous Dataflow (PiSDF)](https://hal.inria.fr/hal-00877492) model.
The purpose of this runtime is to provide rapid-prototyping capabilities to developpers to see how much parallelism they can leverage from their applications.
The framework is developped as a C++ shared library and use as much as possible standard features of the language in order to provide stable experience on different platforms.
Any compiler compliant with the C++11 standard should be able to compile the Spider 2.0 library.

Spider 2.0 is developped at the Institute of Electronics and Telecommunications-Rennes (IETR).

## Overview

* [Build and Install Spider 2.0](#build-and-install-spider-2.0)

* [How to Use the Spider 2.0 Library](#how-to-use-the-spider-2.0-library)

* [Contact](#contact)

* [License](#license)

## Build and Install Spider 2.0

## Build tools

The build process of SPIDER 2.0 relies on [cmake](https://cmake.org) to configure a project for a wide variety of development environments and operating systems. Install [cmake](https://cmake.org/download/) on your system before building the library.

The SPIDER 2.0 code is annotated with the [doxygen](http://www.doxygen.nl/) syntax.  Install [doxygen](http://www.doxygen.nl/download.html) on your system if you want to build the documentation of the library.

### Compiling SPIDER 2.0

To build the shared library, a variety of scripts are provided.

* Building SPIDER 2.0 shared library on Linux using GCC:
* *Note: by default the following does not enable the build of unit tests. To change this behavior, set the BUILD_TESTING option to ON instead of OFF.*
```shell
git clone https://github.com/preesm/spider2.git
cd spider2/bin
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
make spider2 -j$(nproc)
```

* Building SPIDER 2.0 shared library on Linux using CodeBlock:
```shell
git clone https://github.com/preesm/spider2.git
cd spider2/libspider/build-scripts/linux
./CMakeCodeblock.sh
```

* Building SPIDER 2.0 shared library on Windows using CodeBlock:
```shell
git clone https://github.com/preesm/spider2.git
cd spider2/libspider/build-scripts/windows
./CMakeCodeblock.bat
```

* Building SPIDER 2.0 shared library on Windows using Visual Studio 2017:
```shell
git clone https://github.com/preesm/spider2.git
cd spider2/libspider/build-scripts/windows
./CMakeVS2017.bat
```

* **IMPORTANT:** If there is an error during the cmake command on failing to find Gtest framework, look at the README.md in the test subfolder.

### System wide installation

* Linux platforms:
```shell
PREFIX=/usr/local/
sudo cp libSpider2.0.so  ${PREFIX}/lib/
sudo cp libspider/api/*.h ${PREFIX}/include/
```

* Windows platforms:

__(coming soon...)__


### Generating the Doxygen documentation

* Building SPIDER 2.0 documentation on Linux:
```shell
git clone https://github.com/preesm/spider2.git
cd spider2/bin
cmake ..
make doc -j$(nproc)
```

## How to Use the Spider 2.0 Library

* Using CMake based projects:
- Copy the ``` FindSpider2.cmake ``` file from the cmake folder into your project cmake modules directory.
- Add ``` include(FindSpider2) ``` into your CMakeList.txt.
- Add ``` ${SPIDER2_INCLUDE_DIR} ${SPIDER2_INCLUDE_DIR}/api ``` to your include_directories directive.
- Add ``` ${SPIDER2_LIBRARY} ``` to your target_link_library directive.


## Contact

This project is maintained by the Preesm maintainers. Contact us using one of the following:

*   General information : contact@preesm.org
*   Technical support : https://github.com/preesm/spider2/issues

## License

This project is distributed under the CeCILL-C license (see [LICENSE file](LICENSE)).
