#ifndef THREAD_H_
#define THREAD_H_


#include "MSMTUtils.h"
#include "Instruction.h"
#include "Scheduler.h"

using namespace std;

class Thread {
protected:
	unsigned  id;
	int totalInstructions;
	int uOps;
	int nops;
	int lastCycle;
	unsigned windowSize;
	list<Instruction> instructionWindow;
	queue<Instruction> instructionQueue;
	bool createTrace;
	ofstream windowStateFile;
	ofstream instructionsLogFile;
	string runName;
	string traceName;
	ifstream traceFile;
	bool done;

	int parseLine(list<Instruction>& instVec, string line, int instID);
	int parsePtrLine(list<Instruction>& instVec, string line, int instID);
	vector<string> splitLine(string line, char del);
	void parsePtrArray(string ptr, vector<Regs>& regs);
	void calcDependencies();
	void updateReadyInstructions();
	void markDoneInstructions(ExeUnits* exeUnits, int clock);
	void logInstructionTrace(vector<Instruction>& instVec);
	
public:
	Thread(unsigned id, unsigned windowSize, bool createTrace, string runName);
	virtual ~Thread();

	void createTraceFile(const char* fileName);
	void loadBatchOfInstructions();
	int getTotalInstructions();
	int getuOps();
	double getIPC();
	double getuOpsIPC();
	void setLastCycle(int cycle);
	bool threadComplete();
	void print(bool verbose = true);
	void logWindow(int clock);
	void logStat(ofstream& file);

	virtual void updateInstructionWindow(ExeUnits* exeUnits, int clock);
	virtual void scheduleInstructions(ExeUnits* exeUnits, Scheduler* sched, int clock) = 0;

};


class ThreadOrdered : public Thread {

public:
	ThreadOrdered(unsigned id, unsigned windowSize, bool createTrace, string runName);
	virtual ~ThreadOrdered() = default;

	virtual void scheduleInstructions(ExeUnits* exeUnits, Scheduler* sched, int clock);
};


class ThreadOOO : public Thread {

public:
	ThreadOOO(unsigned id, unsigned windowSize, bool createTrace, string runName);
	virtual ~ThreadOOO() = default;

	virtual void scheduleInstructions(ExeUnits* exeUnits, Scheduler* sched, int clock);
};



#endif /* THREAD_H_ */
