Spider
======

Spider (Synchronous Parameterized and Interfaced Dataflow Embedded Runtime) is a dataflow based runtime used to execute dynamic PiSDF applications. The Preesm tool may be used to design PiSDF applications.

## Documentation

Documentation is available online at http://preesm.sourceforge.net/website/ (Tutorial 9 on reconfigurable application).

## Install

The following directives have been tested on Ubuntu 16.04.

### Dependencies

```
sudo apt install git make cmake g++
```

### Compile Spider

```
git clone https://github.com/preesm/spider.git
cd spider/master
./CMakeGCC.sh
```

### System Install

to run as root : 

```
PREFIX=/usr/local/
cp libSpider.so ${PREFIX}/lib/libSpider.so
cp libspider/spider/spider.h ${PREFIX}/include/spider.h
```  

### Applications link

in your GCC command line options, use:

```
gcc ... -lpthread -lSpider ...
```

## Contact

This project is maintained by the Preesm maintainers. Contact us using one of the following:

*   General information : contact@preesm.org
*   Technical support : https://github.com/preesm/spider/issues

## License

This project is distributed under the CeCILL-C license (see [LICENSE file](LICENSE)).
