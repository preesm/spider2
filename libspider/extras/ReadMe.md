# Guideline for Libraries Installation
## Content 

This file contains instructions to setup the libraries in order to compile the
given project files. 

Instructions have been tested for:

* Windows 7 
  * Visual Studio
  * Code::Blocks (MinGW)
  
  
The project compilation requires the following libraries:

* pthread 

## Regarding pthread library, only 2.8.0 is downloadable pre-compiled, 2.10.0 has to be compiled from source.
	However, 2.8.0 doesn't support anything related to thread affinity. Consequently, if using the latter,
	/libspider/platforms/platform_pthread/platformPThread.cpp needs to be edited to comment
	static void setAffinity(int cpuId) function's body.
	Moreover pthread-2.10.0 is compiled using Visual Studio

## pthread-2.10.0
1. Download source code from https://sourceforge.net/projects/pthreads4w/files/latest/download
2. Look for x86 Visual Studio command prompt in the Start menu/All Programs/Visual Studio 20XX/Visual Studio Tools
3. Navigate to pthread-2.10.0 uncompressed sources folder
4. Execute "nmake" to list available command and use the one that fits your needs (nmake clean VC)
5. Copy pthreadVC2.lib to /lib_spider/lib/pthread-2.10.0/lib
   Copy pthreadVC2.dll to /lib_spider/lib/pthread-2.10.0/dll
   Copy _ptw32.h to /lib_spider/lib/pthread-2.10.0/include
   Copy pthread.h to /lib_spider/lib/pthread-2.10.0/include
   Copy sched.h to /lib_spider/lib/pthread-2.10.0/include
   Copy semaphore.h to /lib_spider/lib/pthread-2.10.0/include
   
   
## pthread-2.8.0
1. Download the pthread library:
   [pthread-w32-2-8-0-release.exe](ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-8-0-release.exe)
2. Execute the downloaded executable to decompress its content in a temporary directory.
3. Copy the content of the decompressed /Pre-built.2/ directory into a folder named exactly as follows:
  ```/lib_spider/lib/pthread-2.8.0/```
