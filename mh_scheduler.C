// mh_scheduler.C

#include <float.h>
#include <cstdint>
#include <stdio.h>
#include <vector>
#include <exception>
#include <stdexcept>
#include <assert.h>
#include <stdlib.h>
#include <cmath>
#include <algorithm>

#include "mh_scheduler.h"
#include "mh_random.h"
#include "mh_util.h"

namespace mh {

using namespace std;

int_param lmethod("lmethod","scheduler: 0:no log, 1:normal log, 2:append method name to each entry",2,0,2);


//--------------------------------- Scheduler ---------------------------------------------

void Scheduler::run() {
	checkPopulation();

	timStart = mhtime(_wctime);
	if (timFirstStart == 0)
		timFirstStart = timStart;
	if (lmethod(pgroup)) {
		writeLogHeader();
		writeLogEntry(false,true,"*");

	}

	tmpSol->copy(*pop->at(0));

	if (!terminate()) {
		for(;;) {
			checkPopulation();

			SchedulerMethodResult tmpSolResult;

			SchedulerMethodAndContext mc = getNextMethod(0);
			SchedulerMethod *method = mc.first;
			SchedulerMethodContext *methodContext = mc.second;
			if (methodContext == nullptr)
				methodContext = new SchedulerMethodContext;

			if (mc.first == nullptr || finish) // if in the meanwhile, termination has been started, terminate this thread as well
				break;

			// run the scheduled method
			// methodContext.callCounter has been initialized by getNextMethod
			double startTime=mhcputime();
			method->run(tmpSol, *methodContext, tmpSolResult);
			double methodTime = mhcputime() - startTime;

			// augment missing information in tmpSolResult except tmpSOlResult.reconsider
			if (tmpSolResult.changed) {
				if (tmpSolResult.better == -1)
					tmpSolResult.better = tmpSol->isBetter(*pop->at(0));
				if (tmpSolResult.accept == -1)
					tmpSolResult.accept = tmpSolResult.better;
			}
			else { // unchanged solution
				tmpSolResult.better = false;
				if (tmpSolResult.accept == -1)
					tmpSolResult.accept = false;
			}

			// update statistics and scheduler data
			updateMethodStatistics(pop->at(0),tmpSol,0,methodTime,tmpSolResult);
			updateData(tmpSolResult, 0, true, false);

			if (mc.second == nullptr)
				delete methodContext;

			bool termnow = terminate();	// should we terminate?

			if (!termnow || nIteration>logstr.lastIter())
				writeLogEntry(termnow, true, method->name);

			if (terminate())
				break;
		}
	}

	if (lmethod(pgroup)) {
		logmutex.lock();
		logstr.emptyEntry();
		logstr.flush();
		logmutex.unlock();
	}
}

void Scheduler::reset() {
	mh_advbase::reset();
	finish = false;
}

bool Scheduler::terminate() {
	if (finish)
		return true;
	if (callback != nullptr && callback(pop->bestObj())) {
		finish = true;
		return true;
	}

	// "standard" termination criteria, modified to allow for termination after a certain
	// wall clock time, rather than cpu time, if _wctime is set
	checkPopulation();
	if((_titer >=0 && nIteration>=_titer) ||
		(_tciter>=0 && nIteration-iterBest>=_tciter) ||
		(_tobj >=0 && (_maxi?getBestSol()->obj()>=_tobj:
					getBestSol()->obj()<=_tobj)) ||
		(_ttime>=0 && _ttime<=(mhtime(_wctime) - timStart))) {
		finish = true;
		return true;
	}
	return false;
}

bool Scheduler::terminateMethod() {
	if (finish)
		return true;
	if (callback != nullptr) {
		mutexLock();
		double bobj = pop->bestObj();
		mutexUnlock();
		if (callback != nullptr && callback(bobj)) {
			finish = true;
			return true;
		}
	}
	if (_ttime>=0 && _ttime<=(mhtime(_wctime) - timStart)) {
		finish = true;
		return true;
	}
	return false;
}

void Scheduler::updateMethodStatistics(mh_solution *origsol, mh_solution *tmpsol, int methodIdx,
		double methodTime, SchedulerMethodResult &tmpSolResult) {
	totNetTime[methodIdx] = (totTime[methodIdx] += methodTime);
	nIter[methodIdx]++;
	nIteration++;
	// if the applied method was successful, i.e., is accepted, update the success-counter and the total obj-gain
	if (tmpSolResult.accept) {
		nSuccess[methodIdx]++;
		sumGain[methodIdx] += abs(origsol->obj() - tmpsol->obj());
	}
}

void Scheduler::printMethodStatistics(ostream &ostr) {
	double totSchedulerTime = mhcputime() - timFirstStart;
	ostr << endl << "Scheduler method statistics:" << endl;
	int sumSuccess=0,sumIter=0;
	double sumTime = 0;
	for (int k=0;k<int(methodPool.size());k++) {
		sumSuccess+=nSuccess[k];
		sumIter+=nIter[k];
		sumTime+=totNetTime[k];
	}
	ostr << "total num of completed iterations:\t" << sumIter << endl;
	ostr << "total num of successful iterations:\t" << sumSuccess << endl;
	ostr << "total netto time:\t" << sumTime << "\ttotal scheduler time:\t" << totSchedulerTime << endl;
	ostr << "method\t   iter\t   succ\tsucc-rate%\ttotal-obj-gain\tavg-obj-gain\trel-succ%\ttotal-time\trel-time%\ttot-net-time\trel-net-time%" << endl;
	for (int k = 0; k < int(methodPool.size()); k++) {
		char tmp[250];
		snprintf(tmp,sizeof(tmp),"%7s\t%7d\t%6d\t%9.4f\t%10.5f\t%10.5f\t%9.4f\t%9.4f\t%9.4f\t%9.4f\t%9.4f",
			methodPool[k]->name.c_str(),nIter[k],nSuccess[k],
			double(nSuccess[k])/double(nIter[k])*100.0,
			sumGain[k],
			double(sumGain[k])/double(nIter[k]),
			double(nSuccess[k])/double(sumSuccess)*100.0,
			totTime[k],
			double(totTime[k])/totSchedulerTime*100.0,
			totNetTime[k],
			double(totNetTime[k])/double(sumTime)*100.0);
		ostr << tmp << endl;
	}
	ostr << endl;
}

void Scheduler::printStatistics(ostream &ostr) {
	checkPopulation();

	char s[60];

	double wctime = mhwctime();
	double cputime = mhcputime();

	mh_solution *best=pop->bestSol();
	ostr << "# best solution:" << endl;
	snprintf( s, sizeof(s), nformat(pgroup).c_str(), pop->bestObj() );
	ostr << "best objective value:\t" << s << endl;
	ostr << "best obtained in iteration:\t" << iterBest << endl;
	snprintf( s, sizeof(s), nformat(pgroup).c_str(), timIterBest );
	ostr << "solution time for best:\t" << timIterBest << endl;
	ostr << "best solution:\t";
	best->write(ostr,0);
	ostr << endl;
	ostr << "CPU time:\t" << cputime << "\t\twall clock time:\t" << wctime << endl;
	ostr << "iterations:\t" << nIteration << endl;
	//ostr << "local improvements:\t"  << nLocalImprovements << endl;
	printMethodStatistics(ostr);
}

void Scheduler::writeLogHeader(bool finishEntry) {
	// mh_advbase::writeLogHeader(false);
	// if (lmethod(pgroup))
	// 	logstr.write("method");
	// if (finishEntry)
	//		logstr.finishEntry();

	checkPopulation();
	if (!lmethod(pgroup))
		return;
	logmutex.lock();
	logstr.headerEntry();
	if (ltime(pgroup))
		logstr.write(_wctime ? "wctime" : "cputime");
	if (lmethod(pgroup)==2)
		logstr.write("method");
	if (finishEntry)
		logstr.finishEntry();
	logmutex.unlock();
}


bool Scheduler::writeLogEntry(bool inAnyCase, bool finishEntry, const std::string &method) {
	// if (mh_advbase::writeLogEntry(inAnyCase,false)) {
	//	if (lmethod(pgroup))
	//		logstr.write(method);
	//	if (finishEntry)
	//		logstr.finishEntry();
	//	return true;
	//}
	//return false;

	checkPopulation();
	if (!lmethod(pgroup))
		return false;
	if (logstr.startEntry(nIteration,pop->bestObj(),inAnyCase))
	{
		logmutex.lock();
		if (ltime(pgroup))
			logstr.write(mhtime(_wctime));
		if (lmethod(pgroup)==2)
			logstr.write(method);
		if (finishEntry)
			logstr.finishEntry();
		logmutex.unlock();
		return true;
	}
	return false;
}

void Scheduler::addStatistics(const Scheduler &s) {
	assert(methodPool.size() == s.methodPool.size());
	for (int k = 0; k < int(methodPool.size()); k++) {
		nIter[k] += s.nIter[k];
		totTime[k] += s.totTime[k];
		nSuccess[k] += s.nSuccess[k];
		sumGain[k] += s.sumGain[k];
	}
	timFirstStart = min(timFirstStart,s.timFirstStart);
}

SchedulerMethodAndContext Scheduler::getNextMethod(int idx) {
	assert(methodPool.size()>0);
	return SchedulerMethodAndContext(methodPool[0], nullptr);
}

void Scheduler::updateData(SchedulerMethodResult &tmpSolResult, int idx, bool updateSchedulerData, bool storeResult) {
	// first method has been applied
	if (tmpSolResult.reconsider==0 || (!tmpSolResult.changed && tmpSolResult.reconsider==-1))
		finish = true;
	if (tmpSolResult.accept) {
		// solution to be accepted
		pop->update(0, tmpSol);
	}
	else {
		// unsuccessful call
		// continue with with incumbent VND solution
		if (tmpSolResult.changed)
			tmpSol->copy(*pop->at(0)); // restore incumbent
		return;
	}
}


//--------------------------------- SchedulerMethodSelector ---------------------------------------------

SchedulerMethod *SchedulerMethodSelector::select() {
	if (methodList.empty())
		return nullptr;
	switch (strategy) {
	case MSSequentialRep:
		if (activeSeqRep.empty())
			return nullptr;
		if (lastSeqRep == activeSeqRep.end())
			lastSeqRep = activeSeqRep.begin();
		else {
			lastSeqRep++;
			if (lastSeqRep == activeSeqRep.end())
				lastSeqRep = activeSeqRep.begin();
		}
		lastMethod = *lastSeqRep;
		methodContextList[lastMethod].callCounter++;
		return scheduler->methodPool[methodList[lastMethod]];
	case MSSequentialOnce:
		if (lastMethod == size()-1)
			return nullptr;
		else
			lastMethod++;
		methodContextList[lastMethod].callCounter++;
		return scheduler->methodPool[methodList[lastMethod]];
	case MSRandomRep: {
		if (firstActiveMethod == size())
			return nullptr;
		int r = random_int(firstActiveMethod,methodList.size()-1);
		methodContextList[r].callCounter++;
		lastMethod = r;
		return scheduler->methodPool[methodList[lastMethod]];
	}
	case MSRandomOnce: {
		if (lastMethod == size()-1)
			return nullptr;	// no more methods
		lastMethod++;
		// Choose randomly a not yet selected method and swap it to position lastMethod
		int r = random_int(lastMethod, methodList.size()-1);
		if (r != lastMethod) {
			swap(methodList[lastMethod],methodList[r]);
			swap(methodContextList[lastMethod],methodContextList[r]);
		}
		methodContextList[lastMethod].callCounter++;
		return scheduler->methodPool[methodList[lastMethod]];
	}
	case MSTimeAdaptive: {
		if (firstActiveMethod == size())
			return nullptr;
		//first specify weights for adaptive methods
		double sum = 0;
		int countAdaptive = 0;
		for (int i = firstActiveMethod; i < size(); i++) {
			if (scheduler->methodPool[methodList[i]]->adaptive) {
				double weight = 1000000;
				if (scheduler->nIter[methodList[i]] > 0) {
					double timeFactor = (scheduler->totNetTime[methodList[i]] + 0.01) * (scheduler->totNetTime[methodList[i]] + 0.01);
					weight = scheduler->nIter[methodList[i]] / timeFactor;
				}
				probabilityWeights[i] = weight;
				sum += weight;
				countAdaptive++;
			}
		}

		//specify weights for non-adaptive methods
		double average = 1;
		if (countAdaptive > 0) {
			average = sum / countAdaptive;
		}
		for (int i = firstActiveMethod; i < size(); i++) {
			if (!scheduler->methodPool[methodList[i]]->adaptive) {
				probabilityWeights[i] = average;
				sum += average;
			}
		}

		double r = random_double(0, sum);
		sum = 0;
		for (int i = firstActiveMethod; i < size(); i++) {
			sum += probabilityWeights[i];
			if (r <= sum || i == size() - 1) { //i == size() - 1 for numeric issues
				lastMethod = i;
				methodContextList[lastMethod].callCounter++;
				return scheduler->methodPool[methodList[lastMethod]];
			}
		}
		break;
	}
	case MSSelfadaptive:
		mherror("Selfadaptive strategy in SchedulerMethodSelector::select not yet implemented");
		break;
	default:
		mherror("Invalid strategy in SchedulerMethodSelector::select",tostring(strategy));
	}
	return nullptr;
}


SchedulerMethod *SchedulerMethodSelector::getLastMethod() {
	if (lastMethod == -1)
		return nullptr;
	else
		return scheduler->methodPool[methodList[lastMethod]];
}

void SchedulerMethodSelector::reset(bool hard) {
	lastMethod = -1;
	if (hard) { // also enable disabled methods
		firstActiveMethod = 0;
		for (auto &c : methodContextList)
			c.callCounter = 0;
		if (strategy==MSSequentialRep) {
			activeSeqRep.clear();
			for (int i=0; i<size(); i++)
				activeSeqRep.insert(i);
			lastSeqRep=activeSeqRep.end();
		}
	} else { // soft reset
		if (strategy==MSSequentialRep)
			lastSeqRep=activeSeqRep.end();
	}
}

bool SchedulerMethodSelector::hasFurtherMethod() const {
	switch (strategy) {
	case MSSequentialRep:
		return !activeSeqRep.empty();
	case MSRandomRep:
		return firstActiveMethod < size();
	case MSTimeAdaptive:
		return firstActiveMethod < size();
	default:
		return lastMethod < size()-1;
	}
}

void SchedulerMethodSelector::doNotReconsiderLastMethod() {
	if (lastMethod==-1) return;
	switch (strategy) {
	case MSTimeAdaptive:
	case MSSequentialRep: {
		lastSeqRep = activeSeqRep.erase(lastSeqRep);
		if (lastSeqRep != activeSeqRep.begin())
			--lastSeqRep;
		break;
	}
	case MSRandomRep: {
		int tmp = methodList[lastMethod];
		methodList[lastMethod]=methodList[firstActiveMethod];
		methodList[firstActiveMethod]=tmp;
		lastMethod=firstActiveMethod;
		firstActiveMethod++;
		break;
	}
	default:
		break;
	}
}

} // end of namespace mh

