#include "pch.h"
#include "MyHashTable.h"


MyHashTable::MyHashTable()
{
	termCount = 0;
}


MyHashTable::~MyHashTable()
{
}

void MyHashTable::addTerm(string term)
{
	hashtable[term].termID = termCount;
	termCount++;
}

void MyHashTable::SetStart(string term, int iStart)
{
	hashtable[term].iStart = iStart;
}

void MyHashTable::SetLen(string term, int ilen)
{
	hashtable[term].iLength = ilen;
}

void MyHashTable::SetFreq(string term, int iFreq)
{
	hashtable[term].iFreq = iFreq;
}

void MyHashTable::SetLexicon(string term, Lexicon* lex)
{
	Lexicon* temp = & hashtable[term];
	temp->termID = lex->termID;
	temp->iStart = lex->iStart;
	temp->iLength = lex->iLength;
	temp->iFreq = lex->iFreq;
}

int MyHashTable::GetTermID(string term)
{
	return hashtable[term].termID;
}

Lexicon MyHashTable::GetLexicon(string term)
{
	Lexicon result;
	Lexicon* temp = &hashtable[term];
	result.termID = temp->termID;
	result.iStart = temp->iStart;
	result.iLength= temp->iLength;
	result.iFreq = temp->iFreq;
	return result;
}

int MyHashTable::GetSize()
{
	return termCount;
}

bool MyHashTable::HasLexicon(string term)
{
	return (!(hashtable.find(term) == hashtable.end()));
}

unordered_map<string, Lexicon>* MyHashTable::GetTable()
{
	return &hashtable;
}
