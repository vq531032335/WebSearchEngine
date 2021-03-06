#include "pch.h"
#include "time.h"
#include "MyPageReader.h"

#include <iostream>
#include <string>

using namespace std;
clock_t start, finish;
double totaltime;
void showTime();

int main()
{
	start = clock();


	//before this, you need to put all the uncompressed .wet files into "pages" directory
	MyPageReader manager;

	string isReuse;
	cout << "input Y to use the existing inverted index, otherwise create a new one" << endl;
	cin >> isReuse;
	cin.get();

	if (isReuse == "Y" || isReuse == "y")//use existing inverted index
	{
		manager.startUp();
		
		manager.StartQuery();
		
	}
	else//create a new inverted index based on given pages
	{
		if (manager.StartPosting())
		{
			showTime();

			string mergeDir = manager.StartMerge();

			showTime();

			manager.StartCreateIndex(mergeDir);

			showTime();

			manager.OutputLexDocTable();
			
			manager.StartQuery();
			
		}
	}
	cout << "program exit" << endl;
}

void showTime()
{
	finish = clock();
	totaltime = (double)(finish - start) / CLOCKS_PER_SEC;
	cout << "\nprogram running time: " << totaltime << "s！" << endl;
	start = clock();
}


