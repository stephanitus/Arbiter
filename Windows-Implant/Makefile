all:
	gcc initial.cpp libzmq.dll	-Iinclude	-lwinhttp -liphlpapi	-o bin/initial.exe
	gcc loader.c -lshlwapi -lwininet -lurlmon -o bin/loader.exe