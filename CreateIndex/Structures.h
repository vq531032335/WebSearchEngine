#pragma once
#include <string>
using namespace std;

struct Document
{
	string dUrl;
	int iSize;
	int iStart;
	int iLength;
};

struct DocScore
{
	int iDoc;
	int iTotalFreq;
	float iScore;

	bool operator < (const DocScore& d2) const
	{
		return iScore < d2.iScore;
	}

	bool operator > (const DocScore& d2) const
	{
		return iScore > d2.iScore;
	}
};

struct RawPosting
{
	int iTerm;
	int iPage;
	int iPos;
	int iTempfile;// used to record which file this term comes from

	bool operator < (const RawPosting& v2) const
	{
		if (iTerm == v2.iTerm)
		{
			return iPage < v2.iPage;
		}
		else
		{
			return iTerm < v2.iTerm;
		}
	}

	bool operator > (const RawPosting& v2) const
	{
		if (iTerm == v2.iTerm)
		{
			return iPage > v2.iPage;
		}
		else
		{
			return iTerm > v2.iTerm;
		}
	}
};

struct FinalPosting
{
	int iPage;
	int iFreq;
};

struct Lexicon
{
	int termID;
	int iStart;
	int iLength;
	int iFreq;
};

