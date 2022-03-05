#include "ExeUnits.h"

ExeUnits::ExeUnits(bool createTrace,string runName) : clock(0), createTrace(createTrace), traceFile(), runName(runName) {
	if (createTrace) {
		string fileName = "Out/"+runName+"/Traces/execution_units.log";
		traceFile.open(fileName.c_str());
		if (!traceFile.is_open())
			throw MSMTException("COULD NOT OPEN FILE: " + fileName);
		traceFile << "-----Full Utilization During These Clock Cycles-----" << endl;
	}
};

ExeUnits::~ExeUnits() {
	if (createTrace) {
		traceFile.close();
	}
}

vector<string> ExeUnits::splitLine(string line, char del) {
	vector<string> instArgs;
	stringstream sstream(line);
	string arg;
	while(getline(sstream,arg,del)) {
		instArgs.push_back(arg);
	}
	return instArgs;
}

void ExeUnits::incrementClock() {
	clock++;
}

int ExeUnits::getClock() {
	return clock;
}



HomExeUnits::HomExeUnits(const char* fileName, bool createTrace, string runName) : ExeUnits(createTrace,runName),
		totalExeUnits(0), freeExeUnits(0), cycles(0), utilization(0) {
	ifstream configFile(fileName);
	if (configFile.is_open()) {
		string line;
		getline(configFile,line);
		vector<string> args = this->splitLine(line,' ');
		if (args[1] != "ALL") {
			throw MSMTException("Homogeneous Execution Units: file must contain type ALL");
		}
		totalExeUnits = freeExeUnits = stoi(args[0]);
		cycles = stoi(args[2]);
	} else {
		throw MSMTException("COULD NOT OPEN FILE: " + string(fileName));
	}
}

int HomExeUnits::availableUnits(InstType type) {
	return this->freeExeUnits;
}

bool HomExeUnits::fullyUtilized() {
	return freeExeUnits == 0;
}

bool HomExeUnits::useFreeUnit(Instruction* inst) {
	if (availableUnits(inst->instType) == 0) return false;
	inst->state = iRUNNING;
	inst->cycleDone = clock + cycles;
	this->freeExeUnits--;
	this->utilization += cycles;
	return true;
}

void HomExeUnits::instructionDone(Instruction* inst) {
	inst->state = iDONE;
	this->freeExeUnits++;
	if (freeExeUnits > totalExeUnits) {
		throw MSMTException("Tried to free an unused execution unit");
	}
}

unsigned HomExeUnits::getCycles(InstType type) {
	return (unsigned)cycles;
}

bool HomExeUnits::isHomogeneous() {
	return true;
}

void HomExeUnits::printUtilization(ofstream& statFile) {
	double totalUtil = (double)utilization/(clock*totalExeUnits);
	statFile << "Total Utilization:," << totalUtil << endl;
}

void HomExeUnits::logFullUtilization() {
	if (freeExeUnits == 0) {
		traceFile << "Clock: " << clock << endl;
	}
}


HetExeUnits::HetExeUnits(const char* fileName, bool createTrace, string runName) : ExeUnits(createTrace,runName),
		totalExeUnits(INST_TYPES,0), freeExeUnits(INST_TYPES,0),
		cycles(INST_TYPES,1), utilization(INST_TYPES,0) {
	ifstream configFile(fileName);
	if (configFile.is_open()) {
		string line;
		while(getline(configFile,line)) {
			vector<string> args = this->splitLine(line,' ');
			int instType = (int)strToInstType(args[1]);
			totalExeUnits[instType] = freeExeUnits[instType] = stoi(args[0]);
			cycles[instType] = stoi(args[2]);
		}
		for (int tot : totalExeUnits) {
			if (tot == 0 )
				throw MSMTException("MISSING EXECUTION UNITS!");
		}
	} else {
		throw MSMTException("COULD NOT OPEN FILE: " + string(fileName));
	}
}

int HetExeUnits::availableUnits(InstType type) {
	return this->freeExeUnits[(int)type];
}

bool HetExeUnits::fullyUtilized() {
	for (unsigned i=0; i<freeExeUnits.size(); ++i) {
		if (freeExeUnits[i] != 0) return false;
	}
	return true;
}

bool HetExeUnits::useFreeUnit(Instruction* inst) {
	if (availableUnits(inst->instType) == 0) return false;
	int type = (int)inst->instType;
	inst->state = iRUNNING;
	inst->cycleDone = clock + cycles[type];
	this->freeExeUnits[type]--;
	this->utilization[type] += cycles[type];
	return true;
}

void HetExeUnits::instructionDone(Instruction* inst) {
	int type = (int)(inst->instType);
	inst->state = iDONE;
	this->freeExeUnits[type]++;
	if (freeExeUnits[type] > totalExeUnits[type]) {
		throw MSMTException("Tried to free an unused execution unit");
	}

}

unsigned HetExeUnits::getCycles(InstType type) {
	return (unsigned)cycles[(int)type];
}

bool HetExeUnits::isHomogeneous() {
	return false;
}

void HetExeUnits::printUtilization(ofstream& statFile) {
	unsigned overallUsage = 0, overallExeUnits = 0;
	for (unsigned i = 0; i < totalExeUnits.size() ; ++i ) {
		overallExeUnits += totalExeUnits[i];
		overallUsage += utilization[i];
	}
	double totalUtil = (double)overallUsage/(clock*overallExeUnits);
	statFile << "Total Utilization:, " << totalUtil << "\n";
	statFile << "Utilization by Unit: " << "\n";
	for (unsigned i = 0; i < totalExeUnits.size() ; ++i ) {
		if (totalExeUnits[i]) {
			totalUtil = (double)utilization[i]/(clock*totalExeUnits[i]);
			statFile << instTypeToStr((InstType)i) << ", " << totalUtil << "\n";
		}
	}
}

void HetExeUnits::logFullUtilization() {
	bool firstLog = true;
	for (unsigned i = 0; i < totalExeUnits.size() ; ++i) {
		if(freeExeUnits[i] == 0) {
			if (firstLog) {
				traceFile << "CLOCK: " << clock << "\n";
				firstLog = false;
			}
			traceFile << "  Unit: " << instTypeToStr((InstType)i) << endl;
		}
	}
}
