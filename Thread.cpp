#include "Thread.h"

Thread::Thread(unsigned id, unsigned windowSize, bool createTrace, string runName) : id(id), totalInstructions(0), uOps(0), nops(0), lastCycle(0),
windowSize(windowSize), instructionWindow(), instructionQueue(), createTrace(createTrace), windowStateFile(), instructionsLogFile(), runName(runName), traceName(), traceFile(), done(false) {
	if (createTrace) {
		string fileName = "Out/"+runName+"/Traces/Threads/thread_window_"+to_string(id)+".log";
		windowStateFile.open(fileName);
		if (!windowStateFile.is_open())
			throw MSMTException("COULD NOT OPEN FILE: " + fileName);
			
		fileName = "Out/"+runName+"/Traces/Threads/thread_"+std::to_string(id)+".trc";
		instructionsLogFile.open(fileName);
		if (!instructionsLogFile.is_open())
			throw MSMTException("COULD NOT OPEN FILE: " + fileName);
	}
};

Thread::~Thread() {
	if(createTrace) {
		windowStateFile.close();
		instructionsLogFile.close();
	}
	traceFile.close();
}

void Thread::createTraceFile(const char* fileName) {
	traceName = string(fileName);
	traceFile.open(fileName);
	if(!traceFile.is_open())
		throw MSMTException("COULD NOT OPEN FILE: " + traceName);
}

void Thread::loadBatchOfInstructions() {
	unsigned instructionsInWindow = instructionWindow.size();
	list<Instruction> instVec;
	string line;
	int skipInst = 0;
	while(!done && instructionsInWindow < windowSize) {
		getline(traceFile,line);
		done = traceFile.peek() == EOF;
		if(skipInst) {
			--skipInst;
			++uOps;
		} else {
			if (line.find("SNIPER") != string::npos
				|| line.find("TRACE") != string::npos
				|| line.find("Instruction count") != string::npos) continue;
			skipInst = parseLine(instVec,line,++uOps);
			totalInstructions++;
		}
		instructionsInWindow++;
	}
	for(;skipInst > 0; --skipInst) {
		++uOps;
		getline(traceFile,line);
		done = traceFile.peek() == EOF;
	}
	while (!instVec.empty() && instructionWindow.size() < windowSize) {
		instructionWindow.push_back(instVec.front());
		instVec.pop_front();
		calcDependencies();
	}
	while (!instVec.empty()) {
		instructionQueue.push(instVec.front());
		instVec.pop_front();
	}
}

int Thread::parseLine(list<Instruction>& instVec, string line, int instID) {
	//cout << line << endl;
	if (line.find("nop") != string::npos || line.find("lfence") != string::npos) {
		totalInstructions--;
		nops++;
		uOps--;
		return 0;
		
	} else if (line.find("ptr") != string::npos) {
		return parsePtrLine(instVec, line,instID);
	} else {
		vector<string> instArgs = splitLine(line, ' ');
		if (instArgs[1] == "mov" || instArgs[1] == "cmovz" || instArgs[1] == "cmovnz" || instArgs[1] == "cmovl"
			|| instArgs[1] == "movsxd" || instArgs[1] == "movzx" || instArgs[1] == "cmovb" 
			|| instArgs[1] == "movsx" || instArgs[1] == "cmovbe" || instArgs[1] == "cmovnb"
			|| instArgs[1] == "cmovnbe" || instArgs[1] == "cmovs" || instArgs[1] == "cmovns"
			|| instArgs[1] == "cmovnle" || instArgs[1] == "cmovnl" || instArgs[1] == "cmovle") {
			Instruction inst(instID,MISC);
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			if (instArgs[3].find("0x") == string::npos) {
				inst.regRead.push_back(strToRegs(instArgs[3]));
			}
			if (instArgs[1][0] == 'c') {
				inst.regRead.push_back(FLAGS);
			}
			instVec.push_back(inst);
			return 0;

		} else if ( instArgs[1] == "movdqu" || instArgs[1] == "pmovmskb" || instArgs[1] == "movdqa" 
			|| instArgs[1] == "movd" || instArgs[1] == "punpcklbw" || instArgs[1] == "punpcklwd"
			|| instArgs[1] == "movq" || instArgs[1] == "vpbroadcastb" || instArgs[1] == "vpmovmskb"
			|| instArgs[1] == "punpcklqdq" || instArgs[1] == "punpckldq" || instArgs[1] == "punpckhdq"
			|| instArgs[1] == "punpckhqdq" || instArgs[1] == "vmovd" || instArgs[1] == "vmovq"
			|| instArgs[1] == "vpbroadcastq" || instArgs[1] == "movmskpd" || instArgs[1] == "movapd") {
			Instruction inst(instID,FP);
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			if (instArgs[3].find("0x") == string::npos) {
				inst.regRead.push_back(strToRegs(instArgs[3]));
			}
			instVec.push_back(inst);
			return 0;

		} else if (instArgs[1] == "call" || instArgs[1] == "ret") {
			Instruction inst1(instID,MEM);
			inst1.regWrite.push_back(rsp);
			inst1.regRead.push_back(rsp);
			instVec.push_back(inst1);

			Instruction inst2(instID+1,JUMP_BRANCH);
			if (instArgs.size() >= 3 && instArgs[2].find("0x") == string::npos) {
				inst2.regRead.push_back(strToRegs(instArgs[2]));
			}
			instVec.push_back(inst2);
			return 1;

		} else if (instArgs[1] == "push") {
			Instruction inst(instID,MEM);
			inst.regWrite.push_back(rsp);
			inst.regRead.push_back(rsp);
			if (instArgs[2].find("0x") == string::npos) {
				inst.regRead.push_back(strToRegs(instArgs[2]));
			}
			instVec.push_back(inst);
			return 0;

		} else if (instArgs[1] == "pop") {
			Instruction inst(instID,MEM);
			inst.regWrite.push_back(rsp);
			inst.regRead.push_back(rsp);
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			instVec.push_back(inst);
			return 0;
			
		} else if (instArgs[1] == "add" || instArgs[1] == "sub" || instArgs[1] == "shl" ||
				instArgs[1] == "or" || instArgs[1] == "sar" || instArgs[1] == "shr" ||
				instArgs[1] == "and" ||instArgs[1] == "xor" || instArgs[1] == "sbb" ||
				instArgs[1] == "rol" || instArgs[1] == "blsi" || instArgs[1] == "blsr" ||
				instArgs[1] == "ror" || instArgs[1] == "adc" || instArgs[1] == "popcnt") {
			Instruction inst(instID,ALU);
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			inst.regRead.push_back(strToRegs(instArgs[2]));
			if (instArgs[3].find("0x") == string::npos) {
				inst.regRead.push_back(strToRegs(instArgs[3]));
			}
			inst.regWrite.push_back(FLAGS);
			if (instArgs[1] == "sbb" || instArgs[1] == "adc") {
				inst.regRead.push_back(FLAGS);
			}
			instVec.push_back(inst);
			return 0;

		} else if ( instArgs[1] == "pxor" || instArgs[1] == "pslldq" || instArgs[1] == "psubb" || instArgs[1] == "subsd"
				|| instArgs[1] == "por" || instArgs[1] == "psrldq" || instArgs[1] == "paddd" || instArgs[1] == "mulsd"
				|| instArgs[1] == "pslld" || instArgs[1] == "paddq" || instArgs[1] == "psllq" || instArgs[1] == "divsd") {
			Instruction inst(instID,FP);
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			inst.regRead.push_back(strToRegs(instArgs[2]));
			if (instArgs[3].find("0x") == string::npos) {
				inst.regRead.push_back(strToRegs(instArgs[3]));
			}
			instVec.push_back(inst);
			return 0;

		} else if ( instArgs[1] == "vpxor" || instArgs[1] == "vpcmpeqb" || instArgs[1] == "vxorpd" || instArgs[1] == "vxorps"
				|| instArgs[1] == "vcvtsi2sd" || instArgs[1] == "vmulsd" || instArgs[1] == "vaddsd" || instArgs[1] == "vdivss"
				|| instArgs[1] == "vsubsd" || instArgs[1] == "vdivsd" || instArgs[1] == "vminsd" || instArgs[1] == "vcvtss2sd"
				|| instArgs[1] == "vpor" || instArgs[1] == "vpinsrq" || instArgs[1] == "vinserti128" || instArgs[1] == "vcvtsi2ss"
				|| instArgs[1] == "vpcmpgtb" || instArgs[1] == "vpandn" || instArgs[1] == "vpand" || instArgs[1] == "vfmadd231sd"
				|| instArgs[1] == "vpsubb" || instArgs[1] == "vpslldq" || instArgs[1] == "vpcmpistri" || instArgs[1] == "vsqrtsd"
				|| instArgs[1] == "vdivsd" || instArgs[1] == "vfmadd132sd") {
			Instruction inst(instID,FP);
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			inst.regRead.push_back(strToRegs(instArgs[3]));
			if (instArgs[4].find("0x") == string::npos) {
				inst.regRead.push_back(strToRegs(instArgs[4]));
			}
			inst.regWrite.push_back(FLAGS);
			instVec.push_back(inst);
			return 0;

		} else if (instArgs[1] == "test" || instArgs[1] == "cmp" || instArgs[1] == "bt") {
			Instruction inst(instID,ALU);
			inst.regRead.push_back(strToRegs(instArgs[2]));
			if (instArgs[3].find("0x") == string::npos) {
				inst.regRead.push_back(strToRegs(instArgs[3]));
			}
			inst.regWrite.push_back(FLAGS);
			instVec.push_back(inst);
			return 0;

		} else if (instArgs[1] == "pcmpeqb" || instArgs[1] == "pcmpeqd" || instArgs[1] == "pcmpgtd"
				|| instArgs[1] == "vucomisd" || instArgs[1] == "pcmpistri" || instArgs[1] == "ucomisd") {
			Instruction inst(instID,FP);
			inst.regRead.push_back(strToRegs(instArgs[2]));
			if (instArgs[3].find("0x") == string::npos) {
				inst.regRead.push_back(strToRegs(instArgs[3]));
			}
			if (instArgs[1] == "pcmpistri") {
				inst.regWrite.push_back(rcx);
				inst.regWrite.push_back(FLAGS);
			} else if (instArgs[1] == "vucomisd" || instArgs[1] == "ucomisd") {
				inst.regWrite.push_back(FLAGS);
			} else {
				inst.regWrite.push_back(strToRegs(instArgs[2]));
			}
			instVec.push_back(inst);
			return 0;

		} else if (instArgs[1] == "bsf" || instArgs[1] == "bsr" || instArgs[1] == "tzcnt") {
			Instruction inst(instID,ALU);
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			if (instArgs[3].find("0x") == string::npos) {
				inst.regRead.push_back(strToRegs(instArgs[3]));
			}
			inst.regWrite.push_back(FLAGS);
			instVec.push_back(inst);
			return 0;
			
		} else if (instArgs[1] == "neg" || instArgs[1] == "not" || instArgs[1] == "inc"
				|| instArgs[1] == "bswap" || instArgs[1] == "dec" ) {
			Instruction inst(instID,ALU);
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			inst.regRead.push_back(strToRegs(instArgs[2]));
			instVec.push_back(inst);
			return 0;

		} else if (instArgs[1] == "rdtsc") {
			Instruction inst(instID,MISC);
			inst.regWrite.push_back(rax);
			inst.regWrite.push_back(rdx);
			instVec.push_back(inst);
			return 0;

		} else if (instArgs[1] == "jz" || instArgs[1] == "jbe" || instArgs[1] == "jnbe" || instArgs[1] == "jp"
				|| instArgs[1] == "jnz" || instArgs[1] == "jnb" || instArgs[1] == "jb"
				|| instArgs[1] == "jle" || instArgs[1] == "js" || instArgs[1] == "jnle"
				|| instArgs[1] == "jns" || instArgs[1] == "jl" || instArgs[1] == "jnl") {
			Instruction inst(instID,JUMP_BRANCH);
			inst.regRead.push_back(FLAGS);
			instVec.push_back(inst);
			return 0;

		} else if (instArgs[1] == "jmp") {
			Instruction inst(instID,JUMP_BRANCH);
			instVec.push_back(inst);
			return 0;

		} else if (instArgs[1] == "cdqe" || instArgs[1] == "cdq") {
			Instruction inst(instID,MISC);
			inst.regWrite.push_back(rax);
			inst.regRead.push_back(rax);
			if (instArgs[1] == "cdq") {
				inst.regWrite.push_back(rdx);
			}
			instVec.push_back(inst);
			return 0;
			
		} else if (instArgs[1] == "syscall") {
			Instruction inst(instID,MISC);
			inst.regWrite.push_back(rax);
			inst.regWrite.push_back(rcx);
			inst.regWrite.push_back(r11);
			inst.regRead.push_back(rax);
			inst.regRead.push_back(rdi);
			inst.regRead.push_back(rsi);
			inst.regRead.push_back(rdx);
			inst.regRead.push_back(r8);
			inst.regRead.push_back(r9);
			inst.regRead.push_back(r10);
			instVec.push_back(inst);
			return 0;
			
		} else if (instArgs[1] == "cpuid") {
			Instruction inst(instID,MISC);
			inst.regWrite.push_back(rax);
			inst.regWrite.push_back(rbx);
			inst.regWrite.push_back(rcx);
			inst.regWrite.push_back(rdx);
			instVec.push_back(inst);
			return 0;
			
		} else if(instArgs[1] == "xgetbv") {
			Instruction inst(instID,MISC);
			inst.regRead.push_back(rcx);
			inst.regWrite.push_back(rax);
			inst.regWrite.push_back(rdx);
			instVec.push_back(inst);
			return 0;
			
		} else if (instArgs[1] == "movups") {
			Instruction inst(instID,FP);
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			if (instArgs[3].find("0x") == string::npos) {
				inst.regRead.push_back(strToRegs(instArgs[3]));
			}
			instVec.push_back(inst);
			return 0;
			
		} else if (instArgs[1] == "imul" || instArgs[1] == "mul") {
			Instruction inst(instID,ALU);
			if (instArgs.size() == 3) {
				inst.regRead.push_back(rax);
				inst.regRead.push_back(strToRegs(instArgs[2]));
				inst.regWrite.push_back(rax);
				inst.regWrite.push_back(rdx);
			} else if (instArgs.size() == 4) {
				inst.regRead.push_back(strToRegs(instArgs[2]));
				if (instArgs[3].find("0x") == string::npos) {
					inst.regRead.push_back(strToRegs(instArgs[3]));
				}
				inst.regWrite.push_back(strToRegs(instArgs[2]));
			} else {//5
				inst.regWrite.push_back(strToRegs(instArgs[2]));
				inst.regRead.push_back(strToRegs(instArgs[3]));
			}
			inst.regWrite.push_back(FLAGS);
			instVec.push_back(inst);
			return 0;	
			
		} else if (instArgs[1] == "xchg") {
			Instruction inst(instID,MISC);
			inst.regRead.push_back(strToRegs(instArgs[2]));
			inst.regRead.push_back(strToRegs(instArgs[3]));
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			inst.regWrite.push_back(strToRegs(instArgs[3]));
			instVec.push_back(inst);
			return 0;
			
		} else if (instArgs[1] == "setnz" || instArgs[1] == "setz" || instArgs[1] == "setbe"
				|| instArgs[1] == "setb" || instArgs[1] == "setnle" || instArgs[1] == "setnbe"
				|| instArgs[1] == "setle" || instArgs[1] == "setnp" || instArgs[1] == "setnb") {
			Instruction inst(instID,MISC);
			inst.regRead.push_back(FLAGS);
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			instVec.push_back(inst);
			return 0;
			
		} else if (instArgs[1] == "pshufd" || instArgs[1] == "pshufb") {
			Instruction inst(instID,FP);
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			inst.regRead.push_back(strToRegs(instArgs[3]));
			instVec.push_back(inst);
			return 0;
			
		} else if (instArgs[1] == "div" || instArgs[1] == "idiv") {
			Instruction inst(instID,ALU);
			inst.regRead.push_back(rax);
			inst.regRead.push_back(rdx);
			inst.regRead.push_back(strToRegs(instArgs[2]));
			inst.regWrite.push_back(rax);
			inst.regWrite.push_back(rdx);
			
			inst.regWrite.push_back(FLAGS);
			instVec.push_back(inst);
			return 0;
			
		} else if (instArgs[1] == "shlx" || instArgs[1] == "shrx" || instArgs[1] == "andn") {
			Instruction inst(instID,ALU);
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			inst.regRead.push_back(strToRegs(instArgs[3]));
			inst.regRead.push_back(strToRegs(instArgs[4]));
			instVec.push_back(inst);
			return 0;
			
		} else if (instArgs[1] == "shld" || instArgs[1] == "shrd") {
			Instruction inst(instID,ALU);
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			inst.regRead.push_back(strToRegs(instArgs[2]));
			inst.regRead.push_back(strToRegs(instArgs[3]));
			if (instArgs[4].find("0x") == string::npos) {
				inst.regRead.push_back(strToRegs(instArgs[4]));
			}
			instVec.push_back(inst);
			return 0;
			
		} else if (instArgs[1] == "vzeroupper") {
			Instruction inst(instID,FP);
			inst.regWrite.push_back(ymm0);
			inst.regWrite.push_back(ymm1);
			inst.regWrite.push_back(ymm2);
			inst.regWrite.push_back(ymm3);
			inst.regWrite.push_back(ymm4);
			inst.regWrite.push_back(ymm5);
			inst.regWrite.push_back(ymm6);
			inst.regWrite.push_back(ymm7);
			inst.regWrite.push_back(ymm8);
			inst.regWrite.push_back(ymm9);
			instVec.push_back(inst);
			return 0;
			
		} else if (instArgs[1] == "cqo") {
			Instruction inst(instID,MISC);
			inst.regRead.push_back(rax);
			inst.regWrite.push_back(rax);
			inst.regWrite.push_back(rdx);
			instVec.push_back(inst);
			return 0;
			
		} else if (instArgs[1] == "pminub") {
			Instruction inst(instID,FP);
			
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			inst.regRead.push_back(strToRegs(instArgs[2]));
			inst.regRead.push_back(strToRegs(instArgs[3]));
			
			instVec.push_back(inst);
			return 0;
			
		}
		cerr << "DID NOT PARSE! " << line << endl;
		return 0;
	}
}

int Thread::parsePtrLine(list<Instruction>& instVec, string line, int instID) {
	//cout << line << endl;
	vector<string> instArgs = splitLine(line, ' ');

	if (instArgs[1] == "mov" || instArgs[1] == "lea" || instArgs[1] == "movzx" || instArgs[1] == "movsxd"
		|| instArgs[1] == "movsx" || instArgs[1] == "cmovbe" || instArgs[1] == "cmovz" || instArgs[1] == "cmovnz"
		|| instArgs[1] == "cmovs" || instArgs[1] == "movbe" || instArgs[1] == "cmovnb" || instArgs[1] == "cmovnl"
		|| instArgs[1] == "cmovle" || instArgs[1] == "cmovns") {
		Instruction inst(instID,MEM);
		if (instArgs[2] == "ptr") { //store
			parsePtrArray(instArgs[3],inst.regRead);
			if (instArgs[4].find("0x") == string::npos) {
				inst.regRead.push_back(strToRegs(instArgs[4]));
			}
		} else { //load
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			parsePtrArray(instArgs[4],inst.regRead);
		}
		if (instArgs[1][0] == 'c') {
			inst.regRead.push_back(FLAGS);
		}
		instVec.push_back(inst);
		return 0;
		
	} else if (instArgs[1] == "movdqu" || instArgs[1] == "movdqa" || instArgs[1] == "movaps" || instArgs[1] == "movsd"
			|| instArgs[1] == "movd" || instArgs[1] == "movq" || instArgs[1] == "vmovdqu" || instArgs[1] == "vmovss"
			|| instArgs[1] == "vmovdqa" || instArgs[1] == "vmovsd" || instArgs[1] == "vmovq" || instArgs[1] == "movss") {
		Instruction inst(instID,MEM);
		if (instArgs[2] == "ptr") { //store
			parsePtrArray(instArgs[3],inst.regRead);
			if (instArgs[4].find("0x") == string::npos) {
				inst.regRead.push_back(strToRegs(instArgs[4]));
			}
		} else { //load
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			parsePtrArray(instArgs[4],inst.regRead);
		}
		instVec.push_back(inst);
		return 0;
		
	} else if ( instArgs[1] == "movlpd" || instArgs[1] == "movhpd" || instArgs[1] == "movhps") {
		Instruction inst1(instID,MEM);
		Instruction inst2(instID+1,FP);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[4],inst1.regRead);
		inst2.regWrite.push_back(strToRegs(instArgs[2]));
		
		
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "sub" || instArgs[1] == "add" || instArgs[1] == "or" ||
			instArgs[1] == "xor" || instArgs[1] == "and" || instArgs[1] == "xadd") {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,ALU);
		inst2.dependencies.push_back(instID);
		Instruction inst3(instID+2,MEM); //store
		inst3.dependencies.push_back(instID+1);

		if(instArgs[2] == "ptr") { //with store
			parsePtrArray(instArgs[3],inst1.regRead);
			if (instArgs[4].find("0x") == string::npos) {
				inst2.regRead.push_back(strToRegs(instArgs[4]));
			}
		} else {
			parsePtrArray(instArgs[4],inst1.regRead);
			inst2.regRead.push_back(strToRegs(instArgs[2]));
			inst2.regWrite.push_back(strToRegs(instArgs[2]));
		}
		inst2.regWrite.push_back(FLAGS);
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		if(instArgs[2] == "ptr") {
			instVec.push_back(inst3);
			return 2;
		}
		return 1;
		
	} else if (instArgs[1] == "paddq" || instArgs[1] == "addsd" || instArgs[1] == "andpd" || instArgs[1] == "addss") {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,FP);
		inst2.dependencies.push_back(instID);
		Instruction inst3(instID+2,MEM); //store
		inst3.dependencies.push_back(instID+1);

		if(instArgs[2] == "ptr") { //with store
			parsePtrArray(instArgs[3],inst1.regRead);
			if (instArgs[4].find("0x") == string::npos) {
				inst2.regRead.push_back(strToRegs(instArgs[4]));
			}
		} else {
			parsePtrArray(instArgs[4],inst1.regRead);
			inst2.regRead.push_back(strToRegs(instArgs[2]));
			inst2.regWrite.push_back(strToRegs(instArgs[2]));
		}
		
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		if(instArgs[2] == "ptr") {
			instVec.push_back(inst3);
			return 2;
		}
		return 1;
		
	} else if (instArgs[1] == "pminub") {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,FP);
		inst2.dependencies.push_back(instID);
		
		inst2.regWrite.push_back(strToRegs(instArgs[2]));
		parsePtrArray(instArgs[3],inst1.regRead);
		
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "cmp" || instArgs[1] == "test" ) {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,ALU);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[3],inst1.regRead);
		if (instArgs[4].find("0x") == string::npos) {
			inst2.regRead.push_back(strToRegs(instArgs[4]));
		}
		inst2.regWrite.push_back(FLAGS);
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "pcmpeqb") {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,FP);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[3],inst1.regRead);
		if (instArgs[4].find("0x") == string::npos) {
			inst2.regRead.push_back(strToRegs(instArgs[4]));
		}
		inst2.regWrite.push_back(FLAGS);
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "pcmpistri") {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,FP);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[4],inst1.regRead);
		inst2.regRead.push_back(strToRegs(instArgs[2]));
		
		inst2.regWrite.push_back(FLAGS);
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "vpcmpeqb" || instArgs[1] == "vmulsd" || instArgs[1] == "vmaxsd"
			|| instArgs[1] == "vpinsrq" || instArgs[1] == "vfmadd132sd" || instArgs[1] == "vdivsd") {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,FP);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[5],inst1.regRead);
		if (instArgs[3].find("0x") == string::npos) {
			inst2.regRead.push_back(strToRegs(instArgs[3]));
		}
		inst2.regWrite.push_back(strToRegs(instArgs[2]));

		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "andn" ) {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,ALU);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[5],inst1.regRead);
		inst2.regRead.push_back(strToRegs(instArgs[3]));
		
		inst2.regWrite.push_back(strToRegs(instArgs[2]));
		inst2.regWrite.push_back(FLAGS);
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "vucomisd" || instArgs[1] == "ucomisd") {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,FP);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[4],inst1.regRead);
		inst2.regRead.push_back(strToRegs(instArgs[2]));
		inst2.regWrite.push_back(FLAGS);
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "palignr") {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,FP);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[4],inst1.regRead);
		inst2.regRead.push_back(strToRegs(instArgs[2]));
		inst2.regWrite.push_back(strToRegs(instArgs[2]));
		inst2.regWrite.push_back(FLAGS);
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "vpalignr") {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,FP);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[5],inst1.regRead);
		inst2.regRead.push_back(strToRegs(instArgs[3]));
		inst2.regWrite.push_back(strToRegs(instArgs[2]));
		inst2.regWrite.push_back(FLAGS);
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "jmp") {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,JUMP_BRANCH);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[3],inst1.regRead);
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "call") {
		Instruction inst1(instID,MEM);
		parsePtrArray(instArgs[3],inst1.regRead);
		
		Instruction inst2(instID+1,MEM);
		inst1.regWrite.push_back(rsp);
		inst1.regRead.push_back(rsp);
		instVec.push_back(inst2);

		Instruction inst3(instID+2,JUMP_BRANCH);
		inst3.dependencies.push_back(instID);
		instVec.push_back(inst3);
		
		return 2;
		
	} else if (instArgs[1] == "movups" || instArgs[1] == "vmovups" || instArgs[1] == "vmovaps") {
		Instruction inst(instID,FP);
		if (instArgs[2] == "ptr") { //store
			parsePtrArray(instArgs[3],inst.regRead);
			if (instArgs[4].find("0x") == string::npos) {
				inst.regRead.push_back(strToRegs(instArgs[4]));
			}
		} else { //load
			inst.regWrite.push_back(strToRegs(instArgs[2]));
			parsePtrArray(instArgs[4],inst.regRead);
		}
		instVec.push_back(inst);
		return 0;
		
	} else if (instArgs[1] == "stosb" || instArgs[1] == "stosq") {
		Instruction inst(instID,MEM);
		inst.regRead.push_back(rax);
		parsePtrArray(instArgs[3],inst.regWrite);
		
		instVec.push_back(inst);
		return 0;
		
	} else if (instArgs[1] == "push") {
		Instruction inst1(instID,MEM);
		Instruction inst2(instID+1,MEM);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[3],inst1.regRead);
		inst2.regWrite.push_back(rsp);
		inst2.regRead.push_back(rsp);
		
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;

	} else if (instArgs[1] == "xsavec") {
		Instruction inst1(instID,ALU);
		Instruction inst2(instID+1,MEM);
		inst2.dependencies.push_back(instID);
		
		inst1.regRead.push_back(rax);
		inst1.regRead.push_back(rdx);
		parsePtrArray(instArgs[3],inst2.regRead);
		
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "xrstor") {
		Instruction inst1(instID,MEM);
		Instruction inst2(instID+1,ALU);
		inst2.dependencies.push_back(instID);
		
		inst2.regRead.push_back(rax);
		inst2.regRead.push_back(rdx);
		parsePtrArray(instArgs[3],inst1.regRead);
		
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "cmpxchg") {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,ALU); //compare
		Instruction inst3(instID+2,MEM); //store
		inst2.dependencies.push_back(instID);
		inst3.dependencies.push_back(instID+2);
		
		parsePtrArray(instArgs[3],inst1.regRead);
		inst2.regRead.push_back(rax);
		inst2.regWrite.push_back(FLAGS);
		inst3.regRead.push_back(strToRegs(instArgs[4]));
		
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		instVec.push_back(inst3);
		return 2;
		
	} else if (instArgs[1] == "dec" || instArgs[1] == "inc") {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,ALU); //compare
		Instruction inst3(instID+2,MEM); //store
		inst2.dependencies.push_back(instID);
		inst3.dependencies.push_back(instID+2);
		
		parsePtrArray(instArgs[3],inst1.regRead);
		
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		instVec.push_back(inst3);
		return 2;
		
	} else if (instArgs[1] == "imul") {
		Instruction inst1(instID,MEM);
		Instruction inst2(instID+1,ALU);
		inst2.dependencies.push_back(instID);
		
		if (instArgs.size() == 4) {
			inst2.regRead.push_back(rax);
			parsePtrArray(instArgs[3],inst1.regRead);
			inst2.regWrite.push_back(rax);
			inst2.regWrite.push_back(rdx);
		} else if (instArgs.size() == 5) {
			inst2.regRead.push_back(strToRegs(instArgs[2]));
			parsePtrArray(instArgs[4],inst1.regRead);
			inst2.regWrite.push_back(strToRegs(instArgs[2]));
		} else {//6
			parsePtrArray(instArgs[4],inst1.regRead);
			inst2.regWrite.push_back(strToRegs(instArgs[2]));
		}
		inst2.regWrite.push_back(FLAGS);
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;	
		
	} else if (instArgs[1] == "movsb" || instArgs[1] == "cmpsb" || instArgs[1] == "movsq") {
		Instruction inst1(instID,MEM);
		Instruction inst2(instID+1,MEM);
		inst2.dependencies.push_back(instID);
		Instruction inst3(instID+2,ALU);
		inst3.dependencies.push_back(instID+1);
		
		inst1.regRead.push_back(rsi);
		inst2.regRead.push_back(rdi);
		inst3.regWrite.push_back(rsi);
		inst3.regWrite.push_back(rdi);
		inst3.regRead.push_back(FLAGS);
		
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		instVec.push_back(inst3);
		return 2;
		
	} else if (instArgs[1] == "setnz" || instArgs[1] == "setz" || instArgs[1] == "setbe"
				|| instArgs[1] == "setb" || instArgs[1] == "setnle" || instArgs[1] == "setnbe"
				|| instArgs[1] == "setle" ) {
		Instruction inst1(instID,ALU);
		Instruction inst2(instID+1,MEM);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[3],inst1.regRead);
		inst2.regRead.push_back(FLAGS);
		
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
			
	} else if (instArgs[1] == "fnstcw" || instArgs[1] == "stmxcsr" || instArgs[1] == "fldcw" 
		|| instArgs[1] == "ldmxcsr") {
		Instruction inst1(instID,ALU);
		Instruction inst2(instID+1,MEM);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[3],inst1.regRead);

		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
			
	} else if (instArgs[1] == "shrx") {
		Instruction inst1(instID,MEM);
		Instruction inst2(instID+1,ALU);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[4],inst1.regRead);
		inst2.regRead.push_back(strToRegs(instArgs[5]));
		inst2.regWrite.push_back(strToRegs(instArgs[2]));
		
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "xchg") {
		Instruction inst1(instID,MEM);
		Instruction inst2(instID+1,MEM);
		inst2.dependencies.push_back(instID);
		
		if (instArgs[2] == "ptr") {
			parsePtrArray(instArgs[3],inst1.regRead);
			parsePtrArray(instArgs[3],inst2.regWrite);
			inst2.regWrite.push_back(strToRegs(instArgs[4]));
		} else {
			parsePtrArray(instArgs[4],inst1.regRead);
			parsePtrArray(instArgs[4],inst2.regWrite);
			inst2.regWrite.push_back(strToRegs(instArgs[2]));
		}		
		
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	} else if (instArgs[1] == "bsr" || instArgs[1] == "bsf" || instArgs[1] == "tzcnt") {
		Instruction inst1(instID,MEM); //load
		Instruction inst2(instID+1,ALU);
		inst2.dependencies.push_back(instID);
		
		parsePtrArray(instArgs[4],inst1.regRead);
	
		inst2.regWrite.push_back(strToRegs(instArgs[2]));
		inst2.regWrite.push_back(FLAGS);
		instVec.push_back(inst1);
		instVec.push_back(inst2);
		return 1;
		
	}
	cerr << "DID NOT PARSE! " << line << endl;
	return 0;
}

vector<string> Thread::splitLine(string line, char del) {
	vector<string> instArgs;
	stringstream sstream(line);
	string arg;
	while(getline(sstream,arg,del)) {
		arg.erase(remove(arg.begin(),arg.end(),','),arg.end());
		if (arg == "qword" || arg == "dword" || arg == "byte" || arg == "word" || arg == "xmmword" || arg == "ymmword") continue;
		if (arg == "rep" || arg == "bnd" || arg == "lock") continue;
		instArgs.push_back(arg);
	}
	return instArgs;
}

void Thread::parsePtrArray(string ptr, vector<Regs>& regs) {
	for (unsigned i = 0; i < ptr.length(); ++i) {
		if (ptr[i] == '-') ptr[i] = '+';
	}
	stringstream sstream(ptr.substr(1,ptr.size()-2));
	string tmp;
	while(getline(sstream,tmp,'+')) {
		stringstream stmp(tmp);
		string reg;
		while(getline(stmp,reg,'*')) {
			if (reg.find("0x") != string::npos) continue;
			if (reg.size() < 3) continue;
			regs.push_back(strToRegs(reg));
		}
	}
}

void Thread::calcDependencies() {
	if (instructionWindow.empty()) return;
	list<Instruction>::iterator lastInstIt = instructionWindow.end();
	--lastInstIt;
	list<Regs> readRegs(lastInstIt->regRead.begin(), lastInstIt->regRead.end());
	vector<int>& curDep = lastInstIt->dependencies;
	
	list<Instruction>::reverse_iterator curInstIt = instructionWindow.rbegin();
	++curInstIt;
	while(curInstIt != instructionWindow.rend() && !readRegs.empty() ) {
		for (unsigned k=0; k < curInstIt->regWrite.size(); ++k) {
			list<Regs>::iterator regIt = readRegs.begin();
			while(regIt != readRegs.end()) {
				if ((*regIt) == curInstIt->regWrite[k]) {
					curDep.push_back(curInstIt->instID);
					regIt = readRegs.erase(regIt);
				} else {
					++regIt;
				}
			}
		}
		++curInstIt;
	}
	if(createTrace) {
		lastInstIt->logTrace(instructionsLogFile);
	}
}

void Thread::updateReadyInstructions() {
	if (instructionWindow.size() >= 1) {
		Instruction& inst = instructionWindow.front();
		if (inst.state == iFETCHED)
			inst.state = iREADY;
	}
	if (instructionWindow.size() > 1) {
		list<Instruction>::iterator itF, itB;
		itF = instructionWindow.begin();
		++itF;
		for (; itF != instructionWindow.end(); ++itF) {
			if ((*itF).state != iFETCHED) continue;
			vector<int> dep((*itF).dependencies);
			itB = itF;
			(*itF).state = iREADY;
			do {
				--itB;
				if (find(dep.begin(),dep.end(),(*itB).instID) != dep.end()) {
					(*itF).state = iFETCHED;
				}
			} while (itB != instructionWindow.begin());
		}
	}
}

void Thread::markDoneInstructions(ExeUnits* exeUnits, int clock) {
	for (Instruction& inst : instructionWindow) {
		if (inst.cycleDone == clock) {
			exeUnits->instructionDone(&inst);
		}
	}
}

int Thread::getTotalInstructions() {
	return this->totalInstructions;
}

int Thread::getuOps() {
	return this->uOps;
}

double Thread::getIPC() {
	return (double)totalInstructions/lastCycle;
}

double Thread::getuOpsIPC() {
	return (double)uOps/lastCycle;
}

void Thread::setLastCycle(int cycle) {
	this->lastCycle = cycle;
}

bool Thread::threadComplete() {
	return done && instructionQueue.empty() && instructionWindow.empty();//used to be without done
}

void Thread::print(bool verbose) {
	cout << "Thread: " << id << " Instructions: " << totalInstructions;
	cout << " uOps: " << uOps << " nops: " << nops;
	cout << " Window: " << windowSize << " Cycle Done: " << lastCycle << endl;
}

void Thread::logInstructionTrace(vector<Instruction>& instVec) {
	string fileName = "Out/"+runName+"/Traces/Threads/thread_"+std::to_string(id)+".trc";
	ofstream traceFile(fileName.c_str());
	if (!traceFile.is_open())
		throw MSMTException("COULD NOT OPEN FILE: " + fileName);
	for (Instruction inst: instVec) {
		inst.logTrace(traceFile);
	}
	traceFile.close();
}

void Thread::logWindow(int clock) {
	if(instructionWindow.empty()) return;
	windowStateFile << "Clock: " << clock << "\n";
	for (Instruction inst : instructionWindow) {
		inst.logWindowState(windowStateFile);
	}
}

void Thread::logStat(ofstream& file) {
	file << id << "," << traceName << "," << totalInstructions << "," << uOps << "," << nops << ",";
	file << lastCycle << "," << getIPC() << "," << getuOpsIPC() << endl;
}

ThreadOrdered::ThreadOrdered(unsigned id, unsigned windowSize, bool createTrace, string runName) :
		Thread(id, windowSize, createTrace, runName) {};

void Thread::updateInstructionWindow(ExeUnits* exeUnits, int clock) {
	markDoneInstructions(exeUnits,clock);
	while (!instructionWindow.empty()) {
		Instruction& inst = instructionWindow.front();
		if (inst.state == iDONE) {
			instructionWindow.pop_front();
		} else {
			break;
		}
	}
	while (instructionWindow.size() < windowSize && !instructionQueue.empty()) {
		instructionWindow.push_back(instructionQueue.front());
		instructionQueue.pop();
		calcDependencies();
	}
	loadBatchOfInstructions();
}

void ThreadOrdered::scheduleInstructions(ExeUnits* exeUnits, Scheduler* sched, int clock) {
	if (threadComplete()) return;
	updateInstructionWindow(exeUnits,clock);
	updateReadyInstructions();
	for(Instruction& inst : instructionWindow) {
		if (inst.state == iFETCHED) break;
		if (inst.state != iREADY) continue;
		sched->pushInstruction(id,&inst);
	}
	if (lastCycle == 0 && threadComplete()) {
		lastCycle = clock;
		sched->threadFinished(id);
	}
}


ThreadOOO::ThreadOOO(unsigned id, unsigned windowSize, bool createTrace, string runName) :
		Thread(id, windowSize, createTrace, runName) {};

void ThreadOOO::scheduleInstructions(ExeUnits* exeUnits, Scheduler* sched, int clock) {
	if (threadComplete()) return;
	updateInstructionWindow(exeUnits,clock);
	updateReadyInstructions();
	for(Instruction& inst : instructionWindow) {
		if (inst.state != iREADY) continue;
		sched->pushInstruction(id,&inst);
	}
	if (lastCycle == 0 && threadComplete()) {
		lastCycle = clock;
		sched->threadFinished(id);
	}
}


