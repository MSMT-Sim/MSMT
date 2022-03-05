#include "Instruction.h"


Instruction::Instruction(int instID, InstType instType) : instID(instID),
		instType(instType), regWrite(), regRead(), dependencies(),
		state(iFETCHED), cycleDone(-1) {};

Instruction::Instruction(int instID, InstType instType, vector<Regs> regWrite,
		vector<Regs> regRead) : instID(instID), instType(instType), regWrite(),
		regRead(), dependencies(), state(iFETCHED), cycleDone(-1) {
	for (unsigned i=0; i<regWrite.size(); ++i) {
		this->regWrite.push_back(regWrite[i]);
	}
	for (unsigned i=0; i < regRead.size(); ++i) {
		this->regRead.push_back(regRead[i]);
	}
};

void Instruction::updateDependencies(vector<int> dependencies) {
	for (int i: dependencies) {
		this->dependencies.push_back(i);
	}
}

void Instruction::logTrace(ofstream& file) {
	file << "Instruction ID: " << instID << " Type: " << instTypeToStr(instType);
	file << "\n  Write:";
	for (Regs reg : regWrite) {
		file << " " << regsToStr(reg);
	}
	file << "\n  Read:";
	for (Regs reg : regRead) {
		file << " " << regsToStr(reg);
	}
	file << "\n  Dependencies:";
	for (int i: dependencies) {
		file << " " << i;
	}
	file << endl;
}

void Instruction::logWindowState(ofstream& file) {
	file << "Instruction ID: " << instID << " Type: " << instTypeToStr(instType);
	file << " state: " << instStateToStr(state);
	if (cycleDone >= 0)
		file << "\n  Will Finish Execution During Clock: " << cycleDone;
	file << endl;
}



