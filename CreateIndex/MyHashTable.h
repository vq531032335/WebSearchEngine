#pragma once

#include "Structures.h"
#include <unordered_map>
#include <iostream>
using namespace std;


class MyHashTable
{
public:
	MyHashTable();
	~MyHashTable();


	void SetStart(string term, int iStart);
	void SetLen(string term, int iEnd);
	void SetFreq(string term, int iFreq);
	void SetLexicon(string term, Lexicon* lex);
	void addTerm(string term);
	int GetTermID(string term);
	int GetSize();

	Lexicon GetLexicon(string term);
	bool HasLexicon(string term);
	unordered_map<string, Lexicon>* GetTable();

private:
	int termCount;
	unordered_map<string, Lexicon> hashtable;
};

