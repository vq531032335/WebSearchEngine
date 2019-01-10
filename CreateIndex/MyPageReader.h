#pragma once

#include "MyBufferReader.h"
#include "MyBufferWriter.h"
#include "MyFileReader.h"
#include "MyFileWriter.h"
#include "MyHashTable.h"
#include "InvertedIndexManager.h"
#include "VarByte.h"
#include "Structures.h"
#include <io.h>
#include <iostream>
#include <string>

#include <vector>
#include <queue>
#include <unordered_map>


using namespace std;
typedef priority_queue<RawPosting, vector<RawPosting>, greater<RawPosting>> postingHeapType;

class MyPageReader
//main part of this program
{
public:
	MyPageReader();
	~MyPageReader();


	void GetFiles(string path);//put all the files under path directory into a list called vsfiles
	string LoadFile();//get the name of next unread file. if no file left, returns ""
	   
	bool StartPosting();//read all the pages, put postings into a heap until it is full, then output this heap and loop
	void PagesParse();
	void PostingHeapout(postingHeapType& heap, MyFileWriter<char>* writer);// heap out all the postings into writer's buffer

	string StartMerge();//based on all the posting files we get, merge them into a final posting file

	void StartCreateIndex(string inDir);//based on postings we get, create final inverted index table on disk

	void OutputLexDocTable();//save lexicon and document lists

	void startUp();//read existing lexicon and document files
	void StartQuery();// based on the term we input, show query result

	void PrintSnippet(string term, int docStart, int docLength);
	void CreateTermList();
	bool isEnglish(string& word);

private:
	InvertedIndexManager InvertedIndex;//contain all the tables and infos

	static const int iResultNum = 10;//this number can be changed, which represents how many results we want to get for each query
	static const int iMergeNum = 16;//this number can be changed, which represents how many files we merge each pass
	static const int iQueryMaxNum = 20;//this number can be changed, which represents how many terms we can use in query
	vector<string> vsfiles;//reading files list
	int iLoadFilenum;//reading file No

	vector<string> termList;//temporarily store every term
	//unordered_map<string, int> metTerms;//temporarily store and hash met terms

	static const int iSnippetFrontSize = 3;
	static const int iSnippetBackSize = 5;

	   
	static const int iPostingFileMaxPost = 4000000;//about 30MB// this number can be changed, which means maximum number of postings in each intermediate posting file before merge

};

