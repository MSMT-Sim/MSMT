#include "MSMT.h"


int main(int argc, char **argv) {
	try {
		if (argc < 2) {
			cout << "Must Specify Configuration File!" << endl;
			return 0;
		}
		MSMT msmt(argv[1]);
		cout << "Configuration File: " << argv[1] << endl;
		msmt.printCfg();
		msmt.run();
		msmt.generateStats();
	} catch (MSMTException& e) {
		cout << e.what() << endl;
	}


	return 0;
}

MSMT::MSMT(const char* fileName) : msmtCfg(), exeUnits(), threads(), scheduler() {
	ifstream configFile(fileName);
	if (configFile.is_open()) {
		string line;
		while(getline(configFile,line)) {
			updateConfig(line);
		}
	} else {
		throw MSMTException("COULD NOT OPEN FILE: " + string(fileName));
	}
	initialize();
}

MSMT::~MSMT() {
	delete exeUnits;
	delete scheduler;
	for (Thread* thread : threads) {
		delete thread;
	}
}

vector<string> MSMT::splitLine(string line, char del) {
	vector<string> instArgs;
	stringstream sstream(line);
	string arg;
	while(getline(sstream,arg,del)) {
		instArgs.push_back(arg);
	}
	return instArgs;
}

void MSMT::updateConfig(string line) {
	vector<string> args = this->splitLine(line,' ');
	if (args[0] == "NUM_THREADS:") {
		msmtCfg.numThreads = stoul(args[1]);
	} else if (args[0] == "WINDOW_SIZE:") {
		msmtCfg.windowSize = stoul(args[1]);
	} else if (args[0] == "ORDERING:") {
		msmtCfg.inOrder = (args[1] != "OOO");
	} else if (args[0] == "THREAD_CFG:") {
		msmtCfg.threadCfg = args[1];
	} else if (args[0] == "SCHEDULING:") {
		msmtCfg.schedType = strToSchedType(args[1]);
	} else if (args[0] == "TYPE_EXE_UNITS:") {
		msmtCfg.isHomExeUnits = !(args[1] == "HET");
	} else if (args[0] == "EXE_UNITS_CFG:") {
		msmtCfg.exeUnitsCfg = args[1];
	} else if (args[0] == "RUN_NAME:") {
		msmtCfg.runName = args[1];
	} else if (args[0] == "CREATE_TRACES:") {
		msmtCfg.createTraces = (args[1] == "TRUE");
	}
}

void MSMT::initialize() {
	initializeDirs();
	
	if (msmtCfg.isHomExeUnits) {
		exeUnits = new HomExeUnits(msmtCfg.exeUnitsCfg.c_str(),msmtCfg.createTraces,msmtCfg.runName);
	} else {
		exeUnits = new HetExeUnits(msmtCfg.exeUnitsCfg.c_str(),msmtCfg.createTraces,msmtCfg.runName);
	}

	switch (msmtCfg.schedType) {
	case ICNT:
		scheduler = new SchedulerICnt(exeUnits,msmtCfg.numThreads,msmtCfg.createTraces,msmtCfg.runName);
		break;
	case CCNT:
		scheduler = new SchedulerCCnt(exeUnits,msmtCfg.numThreads,msmtCfg.createTraces,msmtCfg.runName);
		break;
	default:
		scheduler = new SchedulerRR(exeUnits,msmtCfg.numThreads,msmtCfg.createTraces,msmtCfg.runName);
	}

	initializeThreads();
}

void MSMT::initializeDirs() {
	string dirName = "Out/" + msmtCfg.runName;
	//delete Directory if exists
	struct stat st;
	if(stat(dirName.c_str(),&st) == 0) {
		pid_t pid = fork();
		if (pid<0) {
			perror("fork failed");
		} else if (pid == 0) { //child
			char command[1000] = {0};
			string cmd = "rm -rf " + dirName;
			strcpy(command,cmd.c_str());
			char* args[] = {(char*)"/bin/bash", (char*)"-c", command, NULL};
			execv("/bin/bash",args);
			perror("exec failed");
		} else { //parent
			int status;
			waitpid(pid,&status, WUNTRACED);
		}
	}
	//create Directory Tree
	if(mkdir(dirName.c_str(),0777))
		throw MSMTException("COULD NOT CREATE DIR: " + dirName);
	if(msmtCfg.createTraces) {
		dirName += "/Traces";
		if(mkdir(dirName.c_str(),0777))
			throw MSMTException("COULD NOT CREATE DIR: " + dirName);
		dirName += "/Threads";
		if(mkdir(dirName.c_str(),0777))
			throw MSMTException("COULD NOT CREATE DIR: " + dirName);
	}
}

void MSMT::initializeThreads() {
	for (unsigned i = 0; i < msmtCfg.numThreads; ++i) {
		if (msmtCfg.inOrder) {
			threads.push_back(new ThreadOrdered(i,msmtCfg.windowSize,msmtCfg.createTraces,msmtCfg.runName));
		} else {
			threads.push_back(new ThreadOOO(i,msmtCfg.windowSize,msmtCfg.createTraces,msmtCfg.runName));
		}
	}
	unsigned threadsCreated = 0;
	ifstream configFile(msmtCfg.threadCfg.c_str());
	if (configFile.is_open()) {
		string line;
		while(getline(configFile,line)) {
			vector<string> args = this->splitLine(line,' ');
			unsigned threadCnt = stoul(args[0]);
			for (unsigned i=0; i< threadCnt; ++i) {
				threads[threadsCreated]->createTraceFile(args[1].c_str());//loadInstructions(args[1].c_str());
				threadsCreated++;
			}
		}
		if (threadsCreated != msmtCfg.numThreads) {
			throw MSMTException("Did not specify enough threads in " + msmtCfg.threadCfg);
		}
	} else {
		throw MSMTException("COULD NOT OPEN FILE: " + msmtCfg.threadCfg);
	}
}

bool MSMT::done() {
	for (Thread* thread : threads) {
		if (!thread->threadComplete())
			return false;
	}
	return true;
}

void MSMT::run() {
	while(!done()) {
		exeUnits->incrementClock();
		int clock = exeUnits->getClock();
		if(msmtCfg.createTraces) scheduler->logClock(clock);
		for (Thread* thread : threads) {
			thread->scheduleInstructions(exeUnits,scheduler,clock);
		}
		scheduler->schedule();
		if(msmtCfg.createTraces) {
			exeUnits->logFullUtilization();
			for (Thread* thread : threads) {
				thread->logWindow(clock);
			}
		}
	}
}

void MSMT::generateStats() {
	ofstream statFile("Out/"+msmtCfg.runName+"/general_stats.csv");
	if (!statFile.is_open())
		throw MSMTException("COULD NOT OPEN FILE: " + string("general_stats.csv"));
	int totalInstructions = 0, uOps = 0;
	exeUnits->printUtilization(statFile);
	for (Thread* thread : threads) {
		totalInstructions += thread->getTotalInstructions();
		uOps += thread->getuOps();
	}
	statFile << "IPC:," << (double)totalInstructions/exeUnits->getClock() << "\n";
	statFile << "uOps IPC:," << (double)uOps/exeUnits->getClock() << endl;
	statFile.close();

	ofstream threadStatFile("Out/"+msmtCfg.runName+"/thread_stats.csv");
	if(!threadStatFile.is_open())
		throw MSMTException("COULD NOT OPEN FILE: " + string("thread_stats.csv"));
	threadStatFile << "Thread,Trace,x86 Instructions,uOps,nops,Finished at Clock,IPC,uOps IPC" <<endl;
	for (Thread* thread : threads) {
		thread->logStat(threadStatFile);
	}
	threadStatFile.close();
}

void MSMT::printCfg() {
	cout << "MSMT Configurations: " << endl;
	cout << "  Run Name: " << msmtCfg.runName << endl;
	cout << "  Number of Threads: " << msmtCfg.numThreads << endl;
	cout << "  Window Size: " << msmtCfg.windowSize << endl;
	cout << "  In Order: " << boolToStr(msmtCfg.inOrder) << endl;
	cout << "  Thread Configuration File: " << msmtCfg.threadCfg << endl;
	cout << "  Scheduling: " << schedTypeToStr(msmtCfg.schedType) << endl;
	cout << "  Homogeneous Execution Units: " << boolToStr(msmtCfg.isHomExeUnits) << endl;
	cout << "  Execution Units Configuration File: " << msmtCfg.exeUnitsCfg << endl;
	cout << "  Create Traces: " << boolToStr(msmtCfg.createTraces) << endl;
}



