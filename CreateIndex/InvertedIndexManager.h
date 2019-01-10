#pragma once

#include "MyBufferReader.h"
#include "MyBufferWriter.h"
#include "VarByte.h"
#include "MyFileReader.h"
#include "MyHashTable.h"
#include "MyInvertedList.h"
#include "Structures.h"
#include <iostream>
#include <string>

using namespace std;

class InvertedIndexManager
{
public:
	InvertedIndexManager();
	~InvertedIndexManager();
	MyHashTable lexiconTable;//lexicon table

	//static const int iPageMaxNum = 30;//this number can be changed, which controls the number of input pages

	string invertedIndexFile;

	void SetDocUrl(string url);
	void SetDocSize(int size);
	void SetDocStart(int rank);
	void SetDocLength(int length);

	void GetDoc(int docid, Document& doc);
	int GetNowDocStart();

	void DocClear();
	void NextDoc();
	void LastDoc();
	int GetNowDoc();

	int GetDocAvgLen();

	void BindIndex(string filename);
	void SaveDocTable(string filename);
	void SaveLexTable(string filename);
	void LoadDocTable(string filename);
	void LoadLexTable(string filename);


	MyInvertedList* openList(string term);//open the inverted list for term t for reading
	bool closeList(MyInvertedList* lp);//close the inverted list for term t for reading
	int nextGEQ(MyInvertedList* lp, int kDoc);//find the next posting in list lp with docID >= k and return its docID.Return value > MAXDID if none exists.
	int getFreq(MyInvertedList* lp);//get the frequency of the current posting in list lp

	float ComputeBM25(string term, int doc, MyInvertedList* lp, float k1 = 1.2f, float b = 0.75f);
private:
	int iDocAveLen;

	int nowDoc;
	vector<Document> documents;
	//Document documents[iPageMaxNum];//pages table

	
};

