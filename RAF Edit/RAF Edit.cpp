// RAF Edit.cpp : main project file.

#include "stdafx.h"
#include "raf.h"
#include <msclr\marshal_cppstd.h>
#include <stdlib.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#define _CRTDBG_MAP_ALLOC

#include <crtdbg.h>

using namespace System;
using namespace std;
using namespace RAF;
using namespace msclr::interop;

string stdString(String^ s) {
	return marshal_as<std::string>(s);
}

char* cString(String^ s) {
	std::string s_ = marshal_as<std::string>(s);
	return _strdup(s_.c_str());
}

void usage() {
	Console::WriteLine("Usage: ./RAF <mode> input [output] [update]"
		+ "\n\n./RAF update archive.raf newArchive.raf modFolder"
		+ "\n./RAF dump archive.raf"
		+ "\n./RAF extract archive.raf outputFolder");
}

void update(std::string inputRaf, std::string outputRaf, std::string updateFolder) {
	RAFFile r;
	r.read(inputRaf);
	r.update(&updateFolder, &(inputRaf + ".dat"), &(outputRaf + ".dat"));
	r.write(outputRaf);
}

void dump(std::string inputRaf) {
	RAFFile r;
	r.read(inputRaf);
	r.dump();
}

void extract(std::string inputRaf, std::string outputDirectory) {
	RAFFile r;
	r.read(inputRaf);
	r.unpack(&outputDirectory, &(inputRaf + ".dat"));
}

int run(array<System::String ^> ^args)
{
	if (args->Length == 0) {}
	else if (args[0] == "update") {
		update(stdString(args[1]), stdString(args[2]), stdString(args[3]));
		return 0;
	}
	else if (args[0] == "dump") {
		dump(stdString(args[1]));
		return 0;
	}
	else if (args[0] == "extract") {
		extract(stdString(args[1]), stdString(args[2]));
		return 0;
	}
	usage();
	return 1;
}


int main(array<System::String ^> ^args)
{
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	#endif 
	int r = run(args);
	_CrtDumpMemoryLeaks();
	//system("PAUSE");
	return r;
}
