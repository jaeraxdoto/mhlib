# mhlib - An efficient, generic C++ library for metaheuristics #

https://bitbucket.org/ads-tuwien/mhlib

mhlib is a collection of modules supporting the efficient and simple implementation of metaheuristics.

![ ](https://bitbucket.org/ads-tuwien/mhlib/wiki/img/mh.png =10x)

Copyright 2016 G&uuml;nther Raidl <raidl@ac.tuwien.ac.at>,
Algorithms and Complexity Group, TU Wien,
http://www.ac.tuwien.ac.at 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


## Introduction ##

This library is intended to be a problem-independent C++ library
suitable for the development of efficient metaheuristics for 
combinatorial optimization problems.

To use the library you need a GNU C++ compiler and doxygen for the
documentation, call "make all" and have a look at the documentation in
the doxy subdirectory. Under MS Windows, we recommend to use a more
recent version of the mingw-w64 compiler, either in the 32 or 64 bit
version, or the g++ compiler under the cygwin environment.

Note that this library is mainly the result of a collection of 
students projects, and we are not able to guarantee support. 
Although we believe that most modules are stable and efficiently implemented, 
there might be differences in the quality of the code among different modules.
However, we are always happy to get informed about applications,
The library is in development since 1999 at the 
Algorithms and Complexity Group,
Institute of Computer Graphics and Algorithms, TU Wien, Vienna, Austria.
Formerly, it was called EAlib, as it originated from some classes for
evolutionary algorithms.

Mainly responsible for it is *Guenther Raidl* (raidl@ac.tuwien.ac.at),
to whom also belongs the copyright. 
Please report any problems to him. Thank you.

*Daniel Wagner* (d.wagner@cti.ac.at) contributed some local search alike
algorithms and an extensive example for the QAP and many minor changes or
improvements. Further contributions are due to *Sandro Pirkwieser*, *Matthias Prandtstetter*, and *Frederico Dusberger*.

Besides the actual C++ library, mhlib also contains 

- **demo-maxsat**: A template/demonstration program using mhlib and in
  particular its mh_scheduler module for solving the MAXSAT problem by
  means of generalized variable neighborhood search (GVNS). This demo
  should be used as template for new applications realizing a VNS, VND,
  GVNS, GRASP, Large Neighborhood Search and related (hybrid) metaheuristics.
  It also supports multi-threading.

- **demo-sched**: Another demo program using mhlib and in particular its
  mh_scheduler module for solving the ONEMAX and ONEPERM problems by
  means of variable neighborhood search (VNS).  This program also
  includes some specific functions for testing multithreading.

- **summary.pl**: A Perl script used to statistically summarize many runs
  over many instances.

- **aggregate.R**: An R (https://www.r-project.org/) script used to
  further aggregate the results obtained by summary.pl and in particular
  to make statistical tests for showing the significance of different
  configurations. Furthermore, this script also contains some exemplary
  functions for drawing graphs.

- **irace**: An exemplare configuration for applying irace
  (http://iridia.ulb.ac.be/irace/files/README.html) to systematically
  tune parameters. In this example just the number of used VNS
  neighborhoods is tuned for demo-sched. Use this configuration as a
  template for your own mhlib applications. irace must be installed
  within R, and the environment variable IRACE_HOME must be set and
  point to the main irace directory, which is usually 
  /usr/lib/R/site-library/irace or ~/.R_libs/irace.

- **demo-onemax** and **demo-qap**: These are demonstration programs in
  particular for the classes realizing evolutionary algorithms, but also
  simulated annealing and tabu search. The also demonstrated mh_vns and mh_vnd
  are deprecated; new implementations should use mh_scheduler for
  realizing VNS, VND, GRASP, and Large Neighborhood Search. 
  While demo-onemax solves the ONEMAX and ONEPERM problems, demo-qap
  solves the quadratic assignment problem.

## Changelog: major changes over major releases ##

### Version 4.4 ###

- Class mh_solution was made more abstract, all evolutionary
  algorithms related operations were split off into the new gaopsProvider
  class.

- Class mh_advbase was made more abstract, all evolutionary algorithms
  related opeartions were split off into the new subclass
  mh_eaadvbase.

- mherror throws now the newly introduced mh_exception instead of a
  string. You probably want to catch this specific kind of exception 
  in your main program.

### Version 4.3 ###

All parts of the mhlib have been put under the new namespace "mh"; to
compile programs using an older version of mhlib, include `using
namespace "mh";` in your source. Diverse cleaning and improvements in
demo applications, especially demo-sched. Exemplary scripts to use the
automated parameter tuning software http://iridia.ulb.ac.be/irace have
been added in directory irace. Major improvements and fixes in
mh_scheduler (primarily already in versions 4.1 and 4.2). Cleaning and
improvements in the documentation. Diverse
further smaller improvements, which, however, should not affect compatibility.

### Version 4.0 ###

A module mh_scheduler and corresponding demo program demo_sched have been introduced. This module unites and generalizes VND, VNS, GRASP, VLNS and related approaches and provides support for multithreading. It is still under development. Furthermore, some general modules have been refactored.


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
variants of variable neighborhood search, GRASP, and large neighborhood search more efficiently and even with multithreading. For an example see the demo-sched directory.

Then, its probably best to look at the documentation of all the include 
files, which can best be browsed by using doxygen to produce HTML-documentation from the include files (call 'make doc'). Since mhlib has an own parameter handling mechanism and many global parameters controlling the behavior of the library, you should also take a look at them in particular.
