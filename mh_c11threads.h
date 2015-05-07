/*! \file mh_c11threads.h 
	\brief A wrapper including basic C11 thread and mutex classes,
	which also works under MINGW using 
	https://github.com/meganz/mingw-std-threads.
*/

#ifndef MH_C11THREADS_H
#define MH_C11THREADS_H

#if defined(_WIN32)
	#include <Windows.h>
	#include "mingw-std-threads/mingw.thread.h"
	#include <mutex>
	#include "mingw-std-threads/mingw.mutex.h"
    #include "mingw-std-threads/mingw.condition_variable.h"
#else
	#include <thread>
	#include <mutex>
	#include <condition_variable>
#endif

#endif // MH_C11THREADS_H
