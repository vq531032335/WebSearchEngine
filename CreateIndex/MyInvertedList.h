#pragma once
#include "MyFileReader.h"
#include "Structures.h"
#include "VarByte.h"
#include<string>
using namespace std;

class MyInvertedList
{
public:
	MyInvertedList();
	MyInvertedList(string term,int start,int len,string filename);
	~MyInvertedList();

	bool NextPosting();
	FinalPosting nowPosting;
private:
	string termName;
	MyFileReader<char>* indexReader;
};

