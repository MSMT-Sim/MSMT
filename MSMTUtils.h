#ifndef MSMTUTILS_H_
#define MSMTUTILS_H_

#include <vector>
#include <list>
#include <set>
#include <queue>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <utility>
#include <exception>
#include <algorithm>

#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

#define INST_TYPES 5

enum InstType {
	ALU			= 0,
	FP 			= 1,
	MEM			= 2,
	JUMP_BRANCH	= 3,
	MISC		= 4

};

enum Regs {
	rax = 0, rbx, rcx, rdx, rsp, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13,
	r14, r15, rip, FLAGS, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10,
	ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7, ymm8, ymm9
};

enum InstState {
	iFETCHED = 0, iREADY, iSCHEDULED, iRUNNING, iDONE
};

enum SchedType {
	RR = 0, ICNT, CCNT
};

string instTypeToStr(InstType instType);
InstType strToInstType(string str);
string regsToStr(Regs reg);
Regs strToRegs(string str);
string instStateToStr(InstState state);
SchedType strToSchedType(string str);
string schedTypeToStr(SchedType schedType);
string boolToStr(bool val);


class MSMTException : public exception {
	string msg;
public:
	MSMTException(string msg) : msg(msg) {};
	const char* what() const throw () {
		return msg.c_str();
	}
};

#endif /* MSMTUTILS_H_ */
