#include "MSMTUtils.h"

string instTypeToStr(InstType instType) {
	switch (instType) {
		case ALU:
			return "ALU";
		case FP:
			return "FP";
		case MEM:
			return "MEM";
		case JUMP_BRANCH:
			return "JUMP_BRANCH";
		default:
			return "MISC";
	}
}

InstType strToInstType(string str) {
	if (str == "ALU")
		return ALU;
	if (str == "FP")
		return FP;
	if (str == "MEM")
		return MEM;
	if (str == "JUMP_BRANCH")
		return JUMP_BRANCH;
	if (str == "MISC")
		return MISC;
	throw MSMTException("UNKONWN INSTRUCTION TYPE: " + str);
	return MISC;
}

string regsToStr(Regs reg) {
	switch(reg) {
		case rax:
			return "rax";
		case rbx:
			return "rbx";
		case rcx:
			return "rcx";
		case rdx:
			return "rdx";
		case rsp:
			return "rsp";
		case rbp:
			return "rbp";
		case rsi:
			return "rsi";
		case rdi:
			return "rdi";
		case r8:
			return "r8";
		case r9:
			return "r9";
		case r10:
			return "r10";
		case r11:
			return "r11";
		case r12:
			return "r12";
		case r13:
			return "r13";
		case r14:
			return "r14";
		case r15:
			return "r15";
		case rip:
			return "rip";
		case FLAGS:
			return "FLAGS";
		case xmm0:
			return "xmm0";
		case xmm1:
			return "xmm1";
		case xmm2:
			return "xmm2";
		case xmm3:
			return "xmm3";
		case xmm4:
			return "xmm4";
		case xmm5:
			return "xmm5";
		case xmm6:
			return "xmm6";
		case xmm7:
			return "xmm7";
		case xmm8:
			return "xmm8";
		case xmm9:
			return "xmm9";
		case xmm10:
			return "xmm10";
		case ymm0:
			return "ymm0";
		case ymm1:
			return "ymm1";
		case ymm2:
			return "ymm2";
		case ymm3:
			return "ymm3";
		case ymm4:
			return "ymm4";
		case ymm5:
			return "ymm5";
		case ymm6:
			return "ymm6";
		case ymm7:
			return "ymm7";
		case ymm8:
			return "ymm8";
		case ymm9:
			return "ymm9";
		default:
			return "UNKNOWN";
	}
}

Regs strToRegs(string str) {
	if (str.find("ax") != string::npos || str == "al" || str == "ah")
		return rax;
	if (str.find("bx") != string::npos || str == "bl" || str == "bh")
		return rbx;
	if (str.find("cx") != string::npos || str == "cl" || str == "ch")
		return rcx;
	if (str.find("dx") != string::npos || str == "dl" || str == "dh")
		return rdx;
	if (str.find("sp") != string::npos)
		return rsp;
	if (str.find("bp") != string::npos)
		return rbp;
	if (str.find("si") != string::npos)
		return rsi;
	if (str.find("di") != string::npos)
		return rdi;
	if (str.find("r8") != string::npos)
		return r8;
	if (str.find("r9") != string::npos)
		return r9;
	if (str.find("r10") != string::npos)
		return r10;
	if (str.find("r11") != string::npos)
		return r11;
	if (str.find("r12") != string::npos)
		return r12;
	if (str.find("r13") != string::npos)
		return r13;
	if (str.find("r14") != string::npos)
		return r14;
	if (str.find("r15") != string::npos)
		return r15;
	if (str == "rip")
		return rip;
	if (str.find("FLAG") != string::npos)
		return FLAGS;
	if (str == "xmm0")
		return xmm0;
	if (str == "xmm1")
		return xmm1;
	if (str == "xmm2")
		return xmm2;
	if (str == "xmm3")
		return xmm3;
	if (str == "xmm4")
		return xmm4;
	if (str == "xmm5")
		return xmm5;
	if (str == "xmm6")
		return xmm6;
	if (str == "xmm7")
		return xmm7;
	if (str == "xmm8")
		return xmm8;
	if (str == "xmm9")
		return xmm9;
	if (str == "xmm10")
		return xmm10;
	if (str == "ymm0")
		return ymm0;
	if (str == "ymm1")
		return ymm1;
	if (str == "ymm2")
		return xmm2;
	if (str == "ymm3")
		return ymm3;
	if (str == "ymm4")
		return ymm4;
	if (str == "ymm5")
		return ymm5;
	if (str == "ymm6")
		return ymm6;
	if (str == "ymm7")
		return ymm7;
	if (str == "ymm8")
		return ymm8;
	if (str == "ymm9")
		return ymm9;
	throw MSMTException("UNKONWN REGISTER: " + str);
	return FLAGS;
}

string instStateToStr(InstState state) {
	switch(state) {
		case iFETCHED:
			return "FETCHED";
		case iREADY:
			return "READY";
		case iSCHEDULED:
			return "SCHEDULED";
		case iRUNNING:
			return "RUNNING";
		default:
			return "DONE";
	}
}

SchedType strToSchedType(string str) {
	if(str == "ICNT") return ICNT;
	if (str == "CCNT") return CCNT;
	return RR;
}

string schedTypeToStr(SchedType schedType) {
	switch(schedType) {
	case RR:
		return "Round Robin";
	case ICNT:
		return "Instruction Count";
	default:
		return "Cycle Count";
	}
}

string boolToStr(bool val) {
	if(val)
		return "TRUE";
	return "FALSE";
}

