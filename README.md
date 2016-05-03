# mhlib - An efficient, generic C++ library for metaheuristics #

https://bitbucket.org/ads-tuwien/mhlib

*mhlib* is a collection of modules supporting the efficient and simple implementation of metaheuristics in C++11.

![ ](https://bitbucket.org/ads-tuwien/mhlib/wiki/img/mh.png =10x)

Copyright 1999-2016 G&uuml;nther Raidl <raidl@ac.tuwien.ac.at>,
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

To use the library you need a GNU C++11 compiler and doxygen for the
documentation. Under MS Windows, we recommend to use a more recent
version of the mingw-w64 compiler, either in the 32 or 64 bit version,
or the g++ compiler under the cygwin environment.

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

Mainly responsible for it is *Guenther Raidl* <raidl@ac.tuwien.ac.at>,
to whom also belongs the copyright. 
Please report any problems to him. Thank you.

*Daniel Wagner* contributed some local search alike algorithms and an
extensive example for the QAP and many minor changes or improvements.
Further contributions are due to *Sandro Pirkwieser*, *Matthias
Prandtstetter*, *Frederico Dusberger*, *Johannes Maschler*, and *Martin
Riedler*.

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

- **demo-onemax** and **demo-qap**: These are demonstration programs in
  particular for the classes realizing evolutionary algorithms, but also
  simulated annealing and tabu search. The also demonstrated mh_vns and mh_vnd
  are deprecated; new implementations should use mh_scheduler for
  realizing VNS, VND, GRASP, and Large Neighborhood Search. 
  While demo-onemax solves the ONEMAX and ONEPERM problems, demo-qap
  solves the quadratic assignment problem.

- **summary.py**: A Python script used to summarize key result values of
  many runs over many instances.

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
  
- The const specifier of write functions in classes mh_solution, binStringSol 
  and stringSol removed to allow the recomputation of the objective.

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

Clone the project from the git-repository by 

	`git clone git@bitbucket.org:ads-tuwien/mhlib.git`. 

Call 

	`make all; make doc` 
	
from the mhlib-directory.

mhlib has been tested with GNU g++ 4.9.2 under GNU/Linux and Cygwin, but it
should be possible to easily adapt it for other platforms/compilers.
It is based on the C++11 standard.


## Learning to use and extend mhlib ##

To start learning mhlib, it is probably best to first look at **demo-maxsat**.
It shows how to use the most important classes general classes like mh_solution, population, classes for parameter handling, and especially Scheduler for solving the MAXSAT problem by a simple generalized variable
neighborhoods search. You might use this demo as a template for your own
application. **demo-sched** is another example using the
Scheduler for solving the ONEMAX and ONEPERM problem and also provides
some test functionality for multi-threading. 
Especially for genetic algorithms but also the older VNS and the less
actively maintained tabu search and simulated annealing modules, **demo-onemax** and **demo-qap** are recommended as templates.

Then its best to look at the detailed HTML source documentation, which can be built from the source code using *doxygen* by calling `make doc`. Since mhlib has an own parameter handling mechanism and many global parameters controlling the behavior of the library, you should also take a look at them in particular.


## Acknowledgements ##

### mingw-std-threads ###

mhlib includes and uses the library **mingw-std-threads** for multi-threading
under Windows. This library is separately available at
https://github.com/meganz/mingw-std-threads under the
following license:

Copyright (c) 2016, Mega Limited
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


