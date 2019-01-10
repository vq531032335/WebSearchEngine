#include "pch.h"
#include "VarByte.h"


int VBEncode(MyFileWriter<char>* writer, unsigned int num)
//compress a integer into VAR BYTE into the file, returns the length
{

	uint8_t b;
	int result = 0;
	while (num >= 128) {
		int a = num % 128;
		bitset<8> byte(a);
		byte.flip(7);
		num = (num - a) / 128;
		b = (uint8_t)byte.to_ulong();
		//std::cout << byte << endl;
		result++;
		writer->SetStruct(b);

	}
	int a = num % 128;
	bitset<8> byte(a);
	//std::cout << byte << endl;
	b = (uint8_t)byte.to_ulong();
	result++;
	writer->SetStruct(b);
	return result;
}

int VBDecode(MyFileReader<char>* reader)
//uncompress a integer from VAR BYTE from the file
{
	char c;
	int num, p;

	char* t = reader->GetStruct();
	if (t == "" || t == NULL)return -1;
	c = *t;
	delete t;

	bitset<8> byte(c);
	num = 0;
	p = 0;
	while (byte[7] == 1) {
		byte.flip(7);
		num += (int)(byte.to_ulong()*pow(128, p));
		p++;
		char* q = reader->GetStruct();
		c = *q;
		delete q;
		byte = bitset<8>(c);
	}
	num += (int)(byte.to_ulong()*pow(128, p));

	return num;
}

bool ReadRawPosting(MyFileReader<char>* reader, RawPosting* rp)
{
	int t = VBDecode(reader);
	if (t != -1)
	{
		rp->iTerm = t;
		rp->iPage = VBDecode(reader);
		rp->iPos = VBDecode(reader);
		rp->iTempfile = VBDecode(reader);
		return true;
	}
	else
	{
		return false;
	}
}

bool WriteRawPosting(MyFileWriter<char>* writer, RawPosting* rp)
{
	if (rp != NULL)
	{
		VBEncode(writer, rp->iTerm);
		VBEncode(writer, rp->iPage);
		VBEncode(writer, rp->iPos);
		VBEncode(writer, rp->iTempfile);
		return true;
	}
	else
	{
		return false;
		//std::cout << "WriteRawPosting(): RawPosting* is null" << endl;
	}
}

bool ReadFinalPosting(MyFileReader<char>* reader, FinalPosting* fp)
{
	int t = VBDecode(reader);
	if (t != -1)
	{
		fp->iPage = t;
		fp->iFreq = VBDecode(reader);
		return true;
	}
	else
	{
		return false;
	}
}

int WriteFinalPosting(MyFileWriter<char>* writer, FinalPosting* fp)
{
	int len = 0;
	if (fp != NULL)
	{
		len += VBEncode(writer, fp->iPage);
		len += VBEncode(writer, fp->iFreq);
	}
	return len;
}

int ReadTermID(MyFileReader<char>* reader)
{
	return VBDecode(reader);
}

int WriteTermID(MyFileWriter<char>* writer, int termID)
{
	return VBEncode(writer, termID);
}