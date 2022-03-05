CC = g++
OBJS = MSMT.o MSMTUtils.o Instruction.o Thread.o Scheduler.o ExeUnits.o
EXEC = msmtSim
COMP_FLAG = -std=c++11 -Wall

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(EXTRA_FLAG)
MSMT.o: MSMT.cpp MSMT.h Instruction.h MSMTUtils.h Thread.h Scheduler.h ExeUnits.h
	$(CC) -c $(COMP_FLAG) $*.cpp
MSMTUtils.o: MSMTUtils.cpp MSMTUtils.h
	$(CC) -c $(COMP_FLAG) $*.cpp
Instruction.o: Instruction.cpp Instruction.h MSMTUtils.h
	$(CC) -c $(COMP_FLAG) $*.cpp
Thread.o: Thread.cpp Thread.h MSMTUtils.h Instruction.h Scheduler.h
	$(CC) -c $(COMP_FLAG) $*.cpp
Scheduler.o: Scheduler.cpp Scheduler.h MSMTUtils.h Instruction.h ExeUnits.h
	$(CC) -c $(COMP_FLAG) $*.cpp
ExeUnits.o: ExeUnits.cpp ExeUnits.h MSMTUtils.h Instruction.h
	$(CC) -c $(COMP_FLAG) $*.cpp
clean:
	rm -f $(OBJS) $(EXEC)
