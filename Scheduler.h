#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "MSMTUtils.h"
#include "Instruction.h"
#include "ExeUnits.h"

using namespace std;
typedef pair<unsigned,unsigned> CntPair;
typedef set<CntPair> CntSet;

class Scheduler {
protected:
	ExeUnits* exeUnits;
	vector<list<Instruction*>> instructions;
	bool createTrace;
	ofstream traceFile;
	string runName;

	void logTrace(unsigned threadID, int instID);
public:
	Scheduler(ExeUnits* exeUnits, unsigned numThreads, bool createTrace, string runName);
	virtual ~Scheduler();

	virtual void pushInstruction(unsigned threadNum, Instruction* inst);
	virtual void threadFinished(unsigned threadNum) {};
	virtual void schedule() = 0;
	virtual void logClock(int clock);
};

class SchedulerRR : public Scheduler {
	unsigned curThread;
public:
	SchedulerRR(ExeUnits* exeUnits, unsigned numThreads, bool createTrace, string runName);
	virtual ~SchedulerRR() = default;

	void schedule();
};

class SchedulerICnt : public Scheduler {
	CntSet threadICnt;

	bool scheduleInstruction();
public:
	SchedulerICnt(ExeUnits* exeUnits, unsigned numThreads, bool createTrace, string runName);
	virtual ~SchedulerICnt() = default;

	void threadFinished(unsigned threadNum);
	void schedule();
};

class SchedulerCCnt : public Scheduler {
	CntSet threadCCnt;

	bool scheduleInstruction();
public:
	SchedulerCCnt(ExeUnits* exeUnits, unsigned numThreads, bool createTrace, string runName);
	virtual ~SchedulerCCnt() = default;

	void threadFinished(unsigned threadNum);
	void schedule();
};

#endif /* SCHEDULER_H_ */
