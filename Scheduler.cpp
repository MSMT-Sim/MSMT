#include "Scheduler.h"

Scheduler::Scheduler(ExeUnits* exeUnits, unsigned numThreads, bool createTrace, string runName) : exeUnits(exeUnits),
instructions(numThreads), createTrace(createTrace), traceFile(), runName(runName) {
	if (createTrace) {
		string fileName = "Out/"+runName+"/Traces/scheduler.log";
		traceFile.open(fileName.c_str());
		if (!traceFile.is_open())
			throw MSMTException("COULD NOT OPEN FILE: " + fileName);
	}
}

Scheduler::~Scheduler() {
	if(createTrace) {
		traceFile.close();
	}
}

void Scheduler::logTrace(unsigned threadID, int instID) {
	traceFile << "Thread ID: " << threadID << " Instruction ID: " << instID << endl;
}

void Scheduler::logClock(int clock) {
	traceFile << "CLOCK: " << clock << endl;
}

void Scheduler::pushInstruction(unsigned threadNum, Instruction* inst) {
	inst->state = iSCHEDULED;
	this->instructions[threadNum].push_back(inst);
}



SchedulerRR::SchedulerRR(ExeUnits* exeUnits, unsigned numThreads, bool createTrace, string runName) :
		Scheduler(exeUnits,numThreads,createTrace, runName), curThread(numThreads-1) {};

void SchedulerRR::schedule() {
	bool workDone = true;
	unsigned lastThreadScheduled = curThread;
	unsigned numThreads = instructions.size();
	while (workDone) {
		workDone = false;
		for (unsigned i = 1; i <= numThreads; ++i) {
			if(exeUnits->fullyUtilized()) break;
			unsigned threadID = (curThread + i) % numThreads;
			list<Instruction*>& threadQ = instructions[threadID];
			for (list<Instruction*>::iterator it = threadQ.begin(); it != threadQ.end() ; ++it) {
				Instruction* inst = *it;
				if (exeUnits->availableUnits(inst->instType)) {
					//cout << "Thread: " << threadID << " Inst: " << inst->instID << endl;
					logTrace(threadID, inst->instID);
					exeUnits->useFreeUnit(inst);
					threadQ.erase(it);
					workDone = true;
					lastThreadScheduled = threadID;
					break;
				}
			}
		}
	}
	curThread = lastThreadScheduled;
}



SchedulerICnt::SchedulerICnt(ExeUnits* exeUnits, unsigned numThreads, bool createTrace, string runName) :
		Scheduler(exeUnits,numThreads,createTrace,runName), threadICnt() {
	for (unsigned i = 0 ; i < numThreads; ++i) {
		threadICnt.insert(CntPair(0,i));
	}
}

void SchedulerICnt::threadFinished(unsigned threadNum) {
	for(CntSet::iterator itS = threadICnt.begin(); itS != threadICnt.end(); ++itS) {
		if (threadNum == (*itS).second) {
			threadICnt.erase(itS);
			return;
		}
	}
}

void SchedulerICnt::schedule() {
	while(scheduleInstruction());
}

bool SchedulerICnt::scheduleInstruction() {
	if(exeUnits->fullyUtilized()) return false;
	unsigned iCnt, threadID;
	bool didWork = false;
	for(CntSet::iterator itS = threadICnt.begin(); itS != threadICnt.end(); ++itS) {
		threadID = (*itS).second;
		list<Instruction*>& threadQ = instructions[threadID];
		for (list<Instruction*>::iterator it = threadQ.begin(); it != threadQ.end() ; ++it) {
			Instruction* inst = *it;
			if (exeUnits->availableUnits(inst->instType)) {
				iCnt = (*itS).first + 1;
				//cout << "Thread: " << threadID << " Inst: " << inst->instID << endl;
				logTrace(threadID, inst->instID);
				exeUnits->useFreeUnit(inst);
				threadQ.erase(it);
				didWork = true;
				break;
			}
		}
		if(didWork) {
			threadICnt.erase(itS);
			threadICnt.insert(CntPair(iCnt,threadID));
			break;
		}
	}
	return didWork;
}



SchedulerCCnt::SchedulerCCnt(ExeUnits* exeUnits, unsigned numThreads, bool createTrace, string runName) :
		Scheduler(exeUnits,numThreads,createTrace,runName), threadCCnt() {
	for (unsigned i = 0 ; i < numThreads; ++i) {
		threadCCnt.insert(CntPair(0,i));
	}
}

void SchedulerCCnt::threadFinished(unsigned threadNum) {
	for(CntSet::iterator itS = threadCCnt.begin(); itS != threadCCnt.end(); ++itS) {
		if (threadNum == (*itS).second) {
			threadCCnt.erase(itS);
			return;
		}
	}
}

void SchedulerCCnt::schedule() {
	while(scheduleInstruction());
}

bool SchedulerCCnt::scheduleInstruction() {
	if(exeUnits->fullyUtilized()) return false;
	unsigned cycCnt, threadID;
	bool didWork = false;
	for(CntSet::iterator itS = threadCCnt.begin(); itS != threadCCnt.end(); ++itS) {
		threadID = (*itS).second;
		list<Instruction*>& threadQ = instructions[threadID];
		for (list<Instruction*>::iterator it = threadQ.begin(); it != threadQ.end() ; ++it) {
			Instruction* inst = *it;
			if (exeUnits->availableUnits(inst->instType)) {
				cycCnt = (*itS).first + exeUnits->getCycles(inst->instType);
				//cout << "Thread: " << threadID << " Inst: " << inst->instID << endl;
				logTrace(threadID, inst->instID);
				exeUnits->useFreeUnit(inst);
				threadQ.erase(it);
				didWork = true;
				break;
			}
		}
		if(didWork) {
			threadCCnt.erase(itS);
			threadCCnt.insert(CntPair(cycCnt,threadID));
			break;
		}
	}
	return didWork;
}


