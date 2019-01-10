#include "pch.h"
#include "InvertedIndexManager.h"


InvertedIndexManager::InvertedIndexManager()
{
	iDocAveLen = -1;
}


InvertedIndexManager::~InvertedIndexManager()
{
}

void InvertedIndexManager::SetDocUrl(string url)
{
	documents[nowDoc].dUrl = url;
}

void InvertedIndexManager::SetDocSize(int size) 
{
	documents[nowDoc].iSize = size;
}

void InvertedIndexManager::SetDocStart(int rank)
{
	documents[nowDoc].iStart = rank;
}

void InvertedIndexManager::SetDocLength(int length)
{
	documents[nowDoc].iLength= length;
}

void InvertedIndexManager::GetDoc(int docid,Document& doc)
{
	doc = documents[docid];
}

int InvertedIndexManager::GetNowDocStart()
{
	return documents[nowDoc].iStart;
}

void InvertedIndexManager::DocClear()
{
	documents.clear();
	nowDoc = -1;
}

void InvertedIndexManager::NextDoc()
{
	nowDoc++;
	documents.push_back({ "",-1,-1 });

}

void InvertedIndexManager::LastDoc()
{
	nowDoc--;
}

int InvertedIndexManager::GetNowDoc()
{
	return nowDoc;
}

int InvertedIndexManager::GetDocAvgLen()
{
	if (iDocAveLen <= 0)
	{
		int result = 0;
		for (int i = 0; i < (int)documents.size(); i++)
		{
			result += documents[i].iSize;
		}
		iDocAveLen = result / documents.size();
	}
	return iDocAveLen;
}

void InvertedIndexManager::BindIndex(string filename)
{
	invertedIndexFile = filename;
}

void InvertedIndexManager::SaveDocTable(string filename)
{
	MyBufferWriter* document_writer = new MyBufferWriter(filename);
	for (int i = 0; i <(int) documents.size(); i++)
	{
		string doc = documents[i].dUrl + " " + to_string(documents[i].iSize) + " " + to_string(documents[i].iStart)+" " + to_string(documents[i].iLength) + "\n";
		document_writer->SetOneWord(doc);
	}
	document_writer->RefreshBuffer();
	delete document_writer;
}

void InvertedIndexManager::SaveLexTable(string filename)
{
	MyBufferWriter* lexicon_writer = new MyBufferWriter(filename);
	unordered_map<string, Lexicon>* lexTable = lexiconTable.GetTable();
	for (unordered_map<string, Lexicon>::iterator iter = lexTable->begin(); iter != lexTable->end(); iter++)
	{
		string lex = iter->first + " " + to_string(iter->second.termID) + " " + to_string(iter->second.iStart) + " " +
			to_string(iter->second.iLength) + " " + to_string(iter->second.iFreq) + "\n";
		lexicon_writer->SetOneWord(lex);
	}
	lexicon_writer->RefreshBuffer();
	delete lexicon_writer;

}

void InvertedIndexManager::LoadDocTable(string filename)
{
	DocClear();
	MyBufferReader* document_reader = new MyBufferReader(filename);
	string url;
	while ((url = document_reader->GetOneWord()) != "")
	{
		NextDoc();
		SetDocUrl(url);
		int iSize = stoi(document_reader->GetOneWord());
		SetDocSize(iSize);
		int iStart = stoi(document_reader->GetOneWord());
		SetDocStart(iStart);
		int iLength = stoi(document_reader->GetOneWord());
		SetDocLength(iLength);
	}
	delete document_reader;
}

void InvertedIndexManager::LoadLexTable(string filename)
{
	MyBufferReader* lexicon_reader = new MyBufferReader(filename);
	int termCount = 0;
	string term = lexicon_reader->GetOneWord();
	while (term != "")
	{
		lexiconTable.addTerm(term);
		int iTermID = stoi(lexicon_reader->GetOneWord());
		int sStart = stoi(lexicon_reader->GetOneWord());
		int sLen = stoi(lexicon_reader->GetOneWord());
		int sFreq = stoi(lexicon_reader->GetOneWord());
		Lexicon lex = { iTermID, sStart ,sLen ,sFreq };
		lexiconTable.SetLexicon(term, &lex);
		term = lexicon_reader->GetOneWord();
		termCount++;
	}

	delete lexicon_reader;
}


MyInvertedList* InvertedIndexManager::openList(string term)
{
	if (lexiconTable.HasLexicon(term))// if the term exists
	{
		Lexicon p = lexiconTable.GetLexicon(term); // get term info from lexicon table
		MyInvertedList* result = new MyInvertedList(term, p.iStart, p.iLength, invertedIndexFile);
		return result;
	}
	else
	{
		return NULL;
	}
}

bool InvertedIndexManager::closeList(MyInvertedList* lp) 
{
	if (lp != NULL)
	{
		delete lp;
		return true;
	}
	else
	{
		return false;
	}
}

int InvertedIndexManager::nextGEQ(MyInvertedList* lp, int kDoc) 
{
	while (lp->nowPosting.iPage < kDoc)
	{
		if (!lp->NextPosting())
		{
			return INT_MAX;
		}
	} 
	return lp->nowPosting.iPage;
}


int InvertedIndexManager::getFreq(MyInvertedList* lp) 
{
	return lp->nowPosting.iFreq;
}

float InvertedIndexManager::ComputeBM25(string term, int doc, MyInvertedList* lp, float k1, float b)
//N: total number of documents in the collection;
//ft: number of documents that contain term t;
//fdt: frequency of term t in document d;
//dlen: length of document d;
//avgdlen: the average length of documents in the collection;
{
	int N = nowDoc + 1;
	int ft = lexiconTable.GetLexicon(term).iFreq;
	int fdt = getFreq(lp);
	int dlen = documents[doc].iSize;
	int avglen = GetDocAvgLen();

	float K = k1 * ((1 - b) + b * dlen / avglen);
	float result = log((N - ft + 0.5f) / (ft + 0.5f))*(k1 + 1)*fdt / (K + fdt);

	return result;
}