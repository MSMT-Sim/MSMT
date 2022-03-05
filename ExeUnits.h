#ifndef EXEUNITS_H_
#define EXEUNITS_H_

#include "MSMTUtils.h"
#include "Instruction.h"

using namespace std;

class ExeUnits {
protected:
	int clock;
	bool createTrace;
	ofstream traceFile;
	string runName;

	vector<string> splitLine(string line, char del);
public:
	ExeUnits(bool createTrace,string runName);
	virtual ~ExeUnits();

	void incrementClock();
	int getClock();

	virtual int availableUnits(InstType type) = 0;
	virtual bool fullyUtilized() = 0;
	virtual bool useFreeUnit(Instruction* inst) = 0;
	virtual void instructionDone(Instruction* inst) = 0;
	virtual unsigned getCycles(InstType type) = 0;
	virtual bool isHomogeneous() = 0;
	virtual void printUtilization(ofstream& statFile) = 0;
	virtual void logFullUtilization() = 0;
};

class HomExeUnits : public ExeUnits {
	int totalExeUnits;
	int freeExeUnits;
	int cycles;
	int utilization;
public:
	HomExeUnits(const char* fileName, bool createTrace,string runName);
	virtual ~HomExeUnits() = default;

	int availableUnits(InstType type);
	bool fullyUtilized();
	bool useFreeUnit(Instruction* inst);
	void instructionDone(Instruction* inst);
	unsigned getCycles(InstType type);
	bool isHomogeneous();
	void printUtilization(ofstream& statFile);
	void logFullUtilization();
};

class HetExeUnits : public ExeUnits {
	vector<int> totalExeUnits;
	vector<int> freeExeUnits;
	vector<int> cycles;
	vector<int> utilization;
public:
	HetExeUnits(const char* fileName, bool createTrace,string runName);
	virtual ~HetExeUnits() = default;

	int availableUnits(InstType type);
	bool fullyUtilized();
	bool useFreeUnit(Instruction* inst);
	void instructionDone(Instruction* inst);
	unsigned getCycles(InstType type);
	bool isHomogeneous();
	void printUtilization(ofstream& statFile);
	void logFullUtilization();
};


#endif /* EXEUNITS_H_ */
