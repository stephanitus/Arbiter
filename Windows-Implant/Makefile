all:
	g++ -g httpnode.cpp	-Iinclude	-lwinhttp -liphlpapi			-o bin/httpnode.exe
	g++ -g smbnode.cpp	-Iinclude	-liphlpapi						-o bin/smbnode.exe
	g++ -g loader.cpp 				-lshlwapi -lwininet -lurlmon 	-o bin/loader.exe