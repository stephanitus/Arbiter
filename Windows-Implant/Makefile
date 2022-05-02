all:
	g++ httpnode.cpp	-Iinclude	-lwinhttp -liphlpapi			-o bin/httpnode.exe
	g++ smbnode.cpp		-Iinclude	-liphlpapi						-o bin/smbnode.exe
	g++ loader.cpp 					-lshlwapi -lwininet -lurlmon 	-o bin/loader.exe