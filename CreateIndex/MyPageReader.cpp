#include "pch.h"
#include "MyPageReader.h"


MyPageReader::MyPageReader()
{
}

MyPageReader::~MyPageReader()
{
}

void MyPageReader::GetFiles(string path)
//put all the files under path directory into a list called vsfiles
{
	//initialization
	iLoadFilenum = 0;
	vsfiles.clear();

	//get all the files names
	long hFile = 0;
	struct _finddata_t fileinfo;
	string p;
	if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			if ((fileinfo.attrib &  _A_SUBDIR))//if another directory, loop
			{
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
					GetFiles(p.assign(path).append("\\").append(fileinfo.name));
			}
			else//if a file, put into vsfiles(a vector contains type string)
			{
				vsfiles.push_back(p.assign(path).append("\\").append(fileinfo.name));
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
}

string MyPageReader::LoadFile()
//get the name of next unread file. if no file left, returns ""
{
	string result = "";
	if (iLoadFilenum < (int)vsfiles.size())
	{
		result = vsfiles[iLoadFilenum];
		iLoadFilenum++;
	}
	return result;
}

bool MyPageReader::StartPosting()
//read all the pages, put postings into a heap until it is full, then output this heap and loop
{
	std::cout << "\nStartCreatePosting:" << endl;
	
	//initialize file reader
	GetFiles("pages");
	if (vsfiles.size() == 0)
	{
		cout << "no files in pages\\"<< endl;
		return false;
	}

	PagesParse();

	std::cout << "posting complete" << endl;
	return true;
}

void MyPageReader::PagesParse()
{
	int iOutFileCount = 0;//number of files created
	int iHeapSizeCount = 0;//number of postings in heap
	int iPageWordCount = 0;//used to count position
	//int iTermCount = 0; //number of met terms
	int iComLenCount = 0;//point of comppressed pages

	MyFileWriter<char>* compressed_pages_writer = new MyFileWriter<char>("index\\compressed_pages");
	MyFileWriter<char>* posting_writer = new MyFileWriter<char>("postings\\posting" + to_string(iOutFileCount));
	std::cout << "creating intermediate posting file postings\\posting" + to_string(iOutFileCount) << endl;
	InvertedIndex.DocClear();
	postingHeapType postingHeap;//temporarily store and sort postings(only for mode 1)

	//metTerms.clear();

	string sReadTemp;
	while ((sReadTemp = LoadFile()) != "")//if some files remained
	{
		MyBufferReader* page_reader = new MyBufferReader(sReadTemp);

		//initialize
		int iState = -1;
		//some kind of state machine to match the pages
		//-1 before the first useful page
		//0 after "WARC/1.0" 
		//1 after "WARC-Target-URI:"
		//2 after actual URL
		//3 after "Content-Length:"
		//4 after length value
		bool isFirst = true;// to skip the first "WARC/1.0"

		string sWord;
		while ((sWord = page_reader->GetOneWord()) != "")
		{
			//start parser
			if (sWord == "WARC/1.0")
			{
				if (isFirst)
				{
					isFirst = false;
				}
				else
				{
					if (iState == -1 || iState == 4)//if a new doc
					{
						if (iState == 4)//not first
						{
							InvertedIndex.SetDocLength(iComLenCount - InvertedIndex.GetNowDocStart());
						}

						InvertedIndex.NextDoc();
						InvertedIndex.SetDocStart(iComLenCount);

						iState = 0;
						iPageWordCount = 0;
					}
				}
			}
			else if (sWord == "WARC-Target-URI:")
			{
				if (iState == 0)
				{
					iState = 1;
				}
			}
			else if (sWord == "Content-Length:")
			{
				if (iState == 2)
				{
					iState = 3;
				}
			}
			else
			{
				switch (iState)
				{
				case 1:
				{
					//record url
					InvertedIndex.SetDocUrl(sWord);
					iState = 2;
					break;
				}
				case 3:
				{
					InvertedIndex.SetDocSize(stoi(sWord));
					iState = 4;
					break;
				}
				case 4:
				{
					if (!isFirst)
					{
						if (isEnglish(sWord))//is english word
						{

							//handle a term

							if (!InvertedIndex.lexiconTable.HasLexicon(sWord))//if new term
							{
								InvertedIndex.lexiconTable.addTerm(sWord);
							}
							int termID = InvertedIndex.lexiconTable.GetTermID(sWord);
							RawPosting posting = { termID, InvertedIndex.GetNowDoc(), iPageWordCount ,0 };//{termid,docid,termpos}
							if (iHeapSizeCount >= iPostingFileMaxPost)//check if heap is full
							{
								//finish this file
								PostingHeapout(postingHeap, posting_writer);
								posting_writer->RefreshBuffer();
								delete posting_writer;

								//output to another file
								iOutFileCount++;
								posting_writer = new MyFileWriter<char>("postings\\posting" + to_string(iOutFileCount));
								std::cout << "creating intermediate posting file postings\\posting" + to_string(iOutFileCount) << endl;

								//put posting into new heap
								iHeapSizeCount = 0;
							}
							iComLenCount += WriteTermID(compressed_pages_writer, termID);

							postingHeap.push(posting);

							iHeapSizeCount++;
							iPageWordCount++;


						}
					}
					break;
				}
				default:
					break;
				}
			}
		}
		delete page_reader;
	}
	InvertedIndex.SetDocLength(iComLenCount - InvertedIndex.GetNowDocStart());
	PostingHeapout(postingHeap, posting_writer);

	//over
	compressed_pages_writer->RefreshBuffer();
	delete compressed_pages_writer;
	posting_writer->RefreshBuffer();
	delete posting_writer;
}


void MyPageReader::PostingHeapout(postingHeapType& heap, MyFileWriter<char>* writer)
// heap out all the postings into writer's buffer
{
	while (!heap.empty())
	{
		RawPosting p = heap.top();
		WriteRawPosting(writer, &p);
		heap.pop();
	}
}

string MyPageReader::StartMerge()
// based on all the posting files we get, merge them into a final posting file
{
	std::cout << "\nStartMerge:" << endl;

	int iPass = 0;//pass No.
	int iTurn = 0;//turn No. for each pass

	string result;//the file name of the output final posting file

	while (iTurn!=1)// using only 1 turn to merge all the posting files which means merge process is over
	{
		//a pass begins, merge n files to n/16 file
		std::cout << "pass " + to_string( iPass+1) << endl;
		iTurn = 0;

		//read files of each pass
		if (iPass == 0)
		{
			GetFiles("postings");
		}
		else
		{
			GetFiles("postings" + to_string(iPass));
		}

		while (iLoadFilenum < (int)vsfiles.size())// still have files to merge
		{
			//a turn begins, merge 16 files to one file
			std::cout << "turn " + to_string(iTurn + 1)<<" for every "<<to_string(iMergeNum)<<" files" << endl;

			//initialize posting readers
			MyFileReader<char>* posting_reader[iMergeNum];
			int ipostingfilenum = iMergeNum;//actually number of files merged
			for (int i = 0; i < iMergeNum; i++)
			{
				string sFileName = LoadFile();
				if (sFileName == "")//no files to load
				{
					ipostingfilenum = i ;
					break;
				}
				else
				{
					posting_reader[i] = new MyFileReader<char>(sFileName);//bind a reader with a posting file
				}
			}

			//initialize posting writer
			MyFileWriter<char>* posting_writer = new MyFileWriter<char>("postings" + to_string(iPass + 1) + "\\posting" + to_string(iTurn));

			std::cout << "merging into posting file postings" + to_string(iPass + 1) + "\\posting" + to_string(iTurn) << endl;			
			priority_queue<RawPosting, vector<RawPosting>, greater<RawPosting> > termHeap;//a small priority queue binary tree to sort 16 words
			int iEndfilenum = ipostingfilenum;//the number of unfinished files of one turn

			//put first posting of every file into the tree
			for (int i = 0; i < ipostingfilenum; i++)
			{
				RawPosting postTemp;
				ReadRawPosting( posting_reader[i],&postTemp);
				postTemp.iTempfile = i;//record the files which postings came from
				termHeap.push(postTemp);
			}
			while (iEndfilenum > 0)// until all the readers are empty
			{
				int changeFile = termHeap.top().iTempfile;//which file need to give a new posting
				
				//write the smallest posting into writer buffer
				RawPosting rpTemp = termHeap.top();
				WriteRawPosting(posting_writer, &rpTemp);
				termHeap.pop();

				RawPosting nextWord;
				if (ReadRawPosting(posting_reader[changeFile], &nextWord))//if still some postings in this file
				{
					//put the new posting from this file into the tree
					nextWord.iTempfile = changeFile;
					termHeap.push(nextWord);
				}
				else
				{
					iEndfilenum--;//a file is empty
				}
			}

			//close readers and writer
			for (int i = 0; i < ipostingfilenum; i++)
			{
				delete posting_reader[i];
			}
			posting_writer->RefreshBuffer();
			delete posting_writer;
			iTurn++;
		}
		iPass++;
	}

	result = "postings" + to_string(iPass);
	std::cout << "merge complete" << endl;
	return result;
}

void MyPageReader::StartCreateIndex(string inDir)
// based on postings we get, create final inverted index table on disk
{
	std::cout << "\nStartCreateIndex:" << endl;

	MyFileReader<char>* PostingFile = new MyFileReader<char>(inDir + "\\posting0");

	MyFileWriter<char>* invertedIndexFile = new MyFileWriter<char>("index\\inverted_index");
	
	CreateTermList();

	RawPosting lastRP = { -1,0,0,0 };
	RawPosting currentRP;
	int nowPosition = 0;//record position in inverted index
	int nowLength = 0;//record length in inverted index
	int termfreqCount = 1;//number of documents that contain the term
	int freqCount = 1;//frequency of the term in a document
	int addition = 0;
	int postAddition = -1;


	while (ReadRawPosting(PostingFile, &currentRP))
	{
		if (currentRP.iTerm != lastRP.iTerm)//new term
		{
			if (lastRP.iTerm != -1)//not the first term
			{
				// output a final posting due to term changed

				//take differences
				FinalPosting outFP;
				if (postAddition >= 0)
				{
					outFP = { postAddition ,freqCount };
				}
				else
				{
					outFP = { lastRP.iPage ,freqCount };
				}
				postAddition = -1;

				freqCount = 1;

				addition = WriteFinalPosting(invertedIndexFile, &outFP);
				nowPosition += addition;
				nowLength += addition;

				InvertedIndex.lexiconTable.SetFreq(termList[lastRP.iTerm], termfreqCount);
				termfreqCount = 1;

				InvertedIndex.lexiconTable.SetLen(termList[lastRP.iTerm], nowLength);// set last term's length
				nowLength = 0;
			}
			InvertedIndex.lexiconTable.SetStart(termList[currentRP.iTerm], nowPosition);// set current term's start position

		}
		else
		{
			if (lastRP.iPage != currentRP.iPage)//new Doc
			{
				termfreqCount++;

				// output a final posting due to doc changed

				//take differences
				FinalPosting outFP;
				if (postAddition >= 0)
				{
					outFP = { postAddition ,freqCount };
				}
				else
				{
					outFP = { lastRP.iPage ,freqCount };
				}
				postAddition = currentRP.iPage - lastRP.iPage;

				freqCount = 1;

				addition = WriteFinalPosting(invertedIndexFile, &outFP);
				nowPosition += addition;
				nowLength += addition;
			}
			else
			{
				freqCount++;//term's frequency of a Doc increase
			}
		}

		//record the last posting
		lastRP = currentRP;
	}
	//output the final Term info
	FinalPosting outFP;
	if (postAddition >= 0)
	{
		outFP = { postAddition ,freqCount };
	}
	else
	{
		outFP = { lastRP.iPage ,freqCount };
	}
	addition = WriteFinalPosting(invertedIndexFile, &outFP);
	nowPosition += addition;// output a final posting
	InvertedIndex.lexiconTable.SetFreq(termList[lastRP.iTerm], termfreqCount);
	nowLength += addition;
	InvertedIndex.lexiconTable.SetLen(termList[lastRP.iTerm], nowLength);// set last term's end position

	invertedIndexFile->RefreshBuffer();	
	delete PostingFile;
	delete invertedIndexFile;
	std::cout <<"index complete"<< endl;

	InvertedIndex.BindIndex("index\\inverted_index");
}

void MyPageReader::OutputLexDocTable()
//save lexicon and document lists
{
	InvertedIndex.SaveDocTable("index\\documents");

	InvertedIndex.SaveLexTable("index\\lexicon");

	std::cout << "lexicon and documents table complete." << endl;
}

void MyPageReader::startUp()
//read existing lexicon and document files
{
	std::cout << "\nrunning StartUp:" << endl;

	InvertedIndex.LoadDocTable("index\\documents");

	InvertedIndex.LoadLexTable("index\\lexicon");

	CreateTermList();

	InvertedIndex.BindIndex("index\\inverted_index");

	std::cout << "startup complete." << endl;
}

void MyPageReader::StartQuery()
// based on the term we input, show query result
{	
	string searchMode;
	std::cout << "\nplease input search mode( \"and\" conjunctive \"or\" disjunctive otherwise quit )." << endl;
	std::cin >> searchMode;
	std::cin.get();

	while (searchMode == "and" || searchMode == "or")
	{
		std::cout << "\nplease input search terms.     //use Q to quit" << endl;
		string sQuery;
		while (getline(std::cin, sQuery)) {
			if (sQuery == "Q")break;
			bool isFound = false;

			//split terms
			string SearchTerms[20];
			int SearchTermsNum = 0;

			char *pch = NULL;
			char *pnext = NULL;

			pch = strtok_s(&sQuery[0], " ", &pnext);
			while (pch != NULL && SearchTermsNum < 20) {
				SearchTerms[SearchTermsNum] = pch;
				SearchTermsNum++;
				pch = strtok_s(NULL, " ", &pnext);
			}

			//apply queries
			if (SearchTermsNum > 0)
			{
				vector<MyInvertedList*> mylists;//list pointers
				priority_queue<DocScore, vector<DocScore>,greater<DocScore>> scoreHeap;

				if (searchMode == "and")
				{
					//initialize lists
					int candidateDoc = 0;
					bool isOver = false;
					for (int i = 0; i < SearchTermsNum; i++)
					{
						mylists.push_back(InvertedIndex.openList(SearchTerms[i]));
						if (mylists[i] == NULL)
						{
							isOver = true;
						}
					}
					while (!isOver)
					{
						bool isMatch = true;
						//get docID of posting in list0
						candidateDoc = InvertedIndex.nextGEQ(mylists[0], candidateDoc);
						if (candidateDoc > InvertedIndex.GetNowDoc())//beyond overall page number
						{
							isOver = true;
							break;
						}
						for (int i = 1; i < SearchTermsNum; i++)
						{
							//find matching docID
							int temp = InvertedIndex.nextGEQ(mylists[i], candidateDoc);

							if (temp > InvertedIndex.GetNowDoc())//beyond overall page number
							{
								isOver = true;
								break;
							}
							if (temp > candidateDoc)//start again at first list
							{
								isMatch = false;
								candidateDoc = temp;
								break;
							}
						}
						if (!isOver && isMatch)//output together page
						{
							isFound = true;
													   
							float fScore = 0.0;
							int iTotalFreq = 0;
							for (int j = 0; j < SearchTermsNum; j++)
							{
								fScore += InvertedIndex.ComputeBM25(SearchTerms[j], candidateDoc, mylists[j]);
								iTotalFreq += InvertedIndex.getFreq(mylists[j]);
							}
							DocScore candidateScore = { candidateDoc,iTotalFreq ,fScore };
							scoreHeap.push(candidateScore);
							if (scoreHeap.size() > iResultNum)//can be changed
							{
								scoreHeap.pop();
							}
							candidateDoc++;
						}
					}
				}
				else//or mode
				{
					//delete none-exist terms
					int reducednum = 0;
					for (int i = 0; i < SearchTermsNum; i++)
					{
						if (!InvertedIndex.lexiconTable.HasLexicon(SearchTerms[i]))
						{
							SearchTerms[i] = "";
							reducednum++;
						}
					}
					int tempnum = 0;
					for (int i = 0; i < SearchTermsNum; i++)
					{
						if (SearchTerms[i] != "")
						{
							SearchTerms[tempnum] = SearchTerms[i];
							tempnum++;
						}
					}
					SearchTermsNum -= reducednum;


					if (SearchTermsNum != 0)
					{


						//initialize lists
						vector<int> searchTermPage;
						for (int i = 0; i < SearchTermsNum; i++)
						{
							mylists.push_back(InvertedIndex.openList(SearchTerms[i]));
							searchTermPage.push_back(InvertedIndex.nextGEQ(mylists[i], 0));
						}

						while (true)
						{
							//find minimum doc
							int minDoc = searchTermPage[0];
							for (int i = 1; i < SearchTermsNum; i++)
							{
								if (searchTermPage[i] < minDoc)
								{
									minDoc = searchTermPage[i];
								}
							}
							if (minDoc > InvertedIndex.GetNowDoc())//if meet end of all the lists
							{
								break;
							}
							isFound = true;

							//get the score of the doc
							//cout << "doc " << minDoc;
							float fScore = 0.0f;
							int iTotalFreq = 0;
							for (int i = 0; i < SearchTermsNum; i++)
							{
								if (searchTermPage[i] == minDoc)
								{
									fScore += InvertedIndex.ComputeBM25(SearchTerms[i], minDoc, mylists[i]);
									searchTermPage[i] = InvertedIndex.nextGEQ(mylists[i], minDoc + 1);
									iTotalFreq += InvertedIndex.getFreq(mylists[i]);
								}
							}
							DocScore candidateScore = { minDoc ,iTotalFreq,fScore };
							scoreHeap.push(candidateScore);
							if (scoreHeap.size() > iResultNum)
							{
								scoreHeap.pop();
							}
							//cout << " score " << fScore << endl;

						}
					}
				}
				//output the top 10 results from heap
				DocScore topDocList[iResultNum];

				int iResult = 0;
				while (!scoreHeap.empty())
				{
					topDocList[iResult]= scoreHeap.top();

					iResult++;
					scoreHeap.pop();
				}
				for (int i = iResult - 1; i >= 0; i--)
				{
					Document NowDoc;
					InvertedIndex.GetDoc(topDocList[i].iDoc, NowDoc);

					cout << to_string(iResult - i) << "." << NowDoc.dUrl <<" totalFreq: "<< topDocList[i].iTotalFreq << " score:" << topDocList[i].iScore << endl;
					for (int j = 0; j < SearchTermsNum; j++)
					{
						PrintSnippet(SearchTerms[j], NowDoc.iStart, NowDoc.iLength);
					}
					cout << endl;
				}

				for (int i = 0; i < SearchTermsNum; i++)
				{
					InvertedIndex.closeList(mylists[i]);
				}

			}
			if(!isFound)std::cout << "not find any result." << endl;
			std::cout << "\nplease input search terms.     //use Q to quit" << endl;
		}
		std::cout << "\nplease input search mode( \"and\" conjunctive \"or\" disjunctive otherwise quit )." << endl;
		std::cin >> searchMode;
		std::cin.get();
	}

}

void MyPageReader::PrintSnippet(string term, int docStart,int docLength)
{
	int termid = InvertedIndex.lexiconTable.GetTermID(term);
	MyFileReader<char>* cp_reader = new MyFileReader<char>("index\\compressed_pages", docStart, docLength);
	queue<int> snippetList;
	int nowid = 0;
	bool isfind = false;;

	while ((nowid = ReadTermID(cp_reader)) != -1)
	{
		snippetList.push(nowid);
		if (snippetList.size() > iSnippetFrontSize)
		{
			snippetList.pop();
		}
		if (termid == nowid)
		{
			isfind = true;
			break;
		}
	}
	for (int i = 0; i < iSnippetBackSize; i++)
	{
		if ((nowid = ReadTermID(cp_reader)) != -1)
		{
			snippetList.push(nowid);
		}
		else
		{
			break;
		}
	}
	if (isfind)
	{
		cout << "Snippet text: ";
		while (!snippetList.empty())
		{
			int q = snippetList.front();
			cout << termList[q] << " ";
			snippetList.pop();
		}
		cout << endl;
	}

	delete cp_reader;
}

void MyPageReader::CreateTermList()
{
	termList = vector<string>(InvertedIndex.lexiconTable.GetSize());//temporarily store every term
	unordered_map<string, Lexicon>* lexTableP = InvertedIndex.lexiconTable.GetTable();
	for (unordered_map<string, Lexicon>::iterator iter = lexTableP->begin(); iter != lexTableP->end(); iter++)
	{
		termList[iter->second.termID] = iter->first;
	}

}

bool MyPageReader::isEnglish(string& word)
{
	int len = (int)word.length();
	for (int i = 0; i < len;i++)
	{
		if ((word[i] >= 'A' && word[i] <= 'Z'))
		{
			word[i] += 32;
		}
		else if((word[i] >= 'a' && word[i] <= 'z'))
		{
			continue;
		}
		else if (i == len - 1 && (word[i] == '.' || word[i] == ','))
		{
			word.erase(i, 1);
			if (word == "")return false;
		}
		else
		{
			return false;
		}
	}
	return true;
}
