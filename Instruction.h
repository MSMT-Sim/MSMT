
#ifndef INSTRUCTION_H_
#define INSTRUCTION_H_

#include "MSMTUtils.h"

using namespace std;

class Instruction {
public:
	int instID;
	InstType instType;
	vector<Regs> regWrite;
	vector<Regs> regRead;
	vector<int> dependencies;
	InstState state;
	int cycleDone;

	Instruction(int instID, InstType instType);
	Instruction(int instID, InstType instType, vector<Regs> regWrite,
			vector<Regs> regRead);
	~Instruction() = default;

	void updateDependencies(vector<int> dependencies);
	void logTrace(ofstream& file);
	void logWindowState(ofstream& file);
};


#endif /* INSTRUCTION_H_ */
