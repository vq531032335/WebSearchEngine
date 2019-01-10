#include "pch.h"
#include "MyInvertedList.h"


MyInvertedList::MyInvertedList()
{
	indexReader = NULL;
}

MyInvertedList::MyInvertedList(string term, int start,int len, string filename)
{
	MyInvertedList();
	nowPosting = {-1,-1};
	termName = term;
	indexReader = new MyFileReader<char>(filename, start, len);
}



MyInvertedList::~MyInvertedList()
{
	delete indexReader;
}

bool MyInvertedList::NextPosting()
{
	
	FinalPosting newPosting;

	bool result = ReadFinalPosting(indexReader, &newPosting);
	if (nowPosting.iPage == -1)
	{
		nowPosting.iPage = newPosting.iPage;
	}
	else
	{
		nowPosting.iPage += newPosting.iPage;
	}
	nowPosting.iFreq = newPosting.iFreq;

	return result;
	

	//no addition
    //return ReadFinalPosting(indexReader, &nowPosting);
}