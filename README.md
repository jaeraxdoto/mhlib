# mhlib - An efficient, generic C++ library for metaheuristics #

https://bitbucket.org/ads-tuwien/mhlib

mhlib is a collection of modules supporting the efficient and simple implementation of metaheuristics.

![mh.png](https://bitbucket.org/ads-tuwien/mhlib/wiki/img/mh.png =10x)

This library is available under the GNU General Public License Version 3
available at https://www.gnu.org/copyleft/gpl.html

(c) Algorithms and Complexity Structures Group  
Vienna University of Technology  
http://www.ac.tuwien.ac.at  
Mainly responsible is Günther Raidl <raidl@ac.tuwien.ac.at>

To use the library you need a GNU C++ compiler and doxygen for the documentation,
call "make all" and have a look at the documentation in the doxy
subdirectory. Under MS Windows, we recommend to use a more recent version of 
the mingw-w64 compiler, either in the 32 or 64 bit version, or the g++ compiler 
under the cygwin environment.

Note that this library is mainly the result of a collection of 
students projects, and we are not able to guarantee support. 
Although we believe that most modules are stable and efficiently implemented, 
there might be differences in the quality of the code among different modules.

However, we are always happy to get informed about applications,
potential errors or suggestions for improvement.


## Introduction ##

This library is intended to be a problem-independent C++ library
suitable for the development of efficient metaheuristics for 
combinatorial optimization problems.

The library is in development since 1999 at the Vienna University of Technology,
Institute of Computer Graphics and Algorithms, Vienna, Austria.
Formerly, it was called EAlib, as it originated from some classes for
evolutionary algorithms.

Mainly responsible for it is *Guenther Raidl* (raidl@ac.tuwien.ac.at),
to whom also belongs the copyright. 
Please report any problems to him. Thank you.

*Daniel Wagner* (d.wagner@cti.ac.at) contributed some local search alike
algorithms and an extensive example for the QAP and many minor changes or
improvements. Further contributions are due to *Sandro Pirkwieser* and *Matthias Prandtstetter*.

## Installing mhlib ##

Unpack the .tgz or .zip file in a directory or clone the project from the git-repository. Call "make all" in the mhlib-directory.

mhlib has been tested with GNU g++ 4.9.2 under GNU/Linux and Cygwin, but it
should be possible to easily adapt it for other platforms/compilers.
It is based on the C++11 standard.

## Learning to use and extend mhlib ##

To start learning mhlib, it is probably best to look at the
simple exemplary main programs in demo-onemax.
It shows how to use parameters and the most important classes of the library on the simple ONEMAX and ONEPERM problem.

A more complex example for the quadratic assignment problem is provided
in the demo-qap directory.

More recently, in version 4.0, a Scheduler module has been added that supports
variants of variable neighborhood search, GRASP, and large neighborhood search more efficiently and even with multithreading. For an example see the demo-schedtest directory.

Then, its probably best to look at the documentation of all the include 
files, which can best be browsed by using doxygen to produce HTML-documentation from the include files (call 'make doc'). Since mhlib has an own parameter handling mechanism and many global parameters controlling the behavior of the library, you should also take a look at them in particular.
