#ifndef MSMT_H_
#define MSMT_H_

#include "Instruction.h"
#include "MSMTUtils.h"
#include "Thread.h"
#include "Scheduler.h"
#include "ExeUnits.h"

using namespace std;

struct MSMTConfig {
	string runName;
	unsigned numThreads;
	unsigned windowSize;
	bool inOrder;
	string threadCfg;
	SchedType schedType;
	bool isHomExeUnits;
	string exeUnitsCfg;
	bool createTraces;
};


class MSMT {
	MSMTConfig msmtCfg;
	ExeUnits* exeUnits;
	vector<Thread*> threads;
	Scheduler* scheduler;

	vector<string> splitLine(string line, char del);
	void updateConfig(string line);
	void initialize();
	void initializeDirs();
	void initializeThreads();
public:
	MSMT(const char* fileName);
	~MSMT();

	bool done();
	void run();
	void generateStats();
	void printCfg();
};


#endif /* MSMT_H_ */
