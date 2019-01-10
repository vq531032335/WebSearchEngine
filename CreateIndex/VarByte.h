#pragma once
#include "MyFileWriter.h"
#include "MyFileReader.h"
#include "Structures.h"
#include <bitset>

using namespace std;

int VBEncode(MyFileWriter<char>* writer, unsigned int num);

int VBDecode(MyFileReader<char>* reader);

bool ReadRawPosting(MyFileReader<char>* reader, RawPosting* rp);

bool WriteRawPosting(MyFileWriter<char>* writer, RawPosting* rp);

bool ReadFinalPosting(MyFileReader<char>* reader, FinalPosting* fp);

int WriteFinalPosting(MyFileWriter<char>* writer, FinalPosting* fp);

int ReadTermID(MyFileReader<char>* reader);

int WriteTermID(MyFileWriter<char>* writer, int termID);

