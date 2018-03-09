#include <iostream>
#include <cstdlib>
#include <vector>
#include <stdio.h>
#include <fstream>
#include <cstring>

using namespace std;

//	***********************
//		Declare TLB
//	***********************

struct tlbUnit{
	int pageNumber;
	int frameNumber;
	int haveNum;
	int lastTimeUsed;
};
struct tlbUnit TLB[16];

//	***********************
//	   Declare Page Table
//	***********************

struct pagetableUnit{
	int frameNumber;
	int validBit;
};
struct pagetableUnit pageTable[256];

//	***********************
//		Declare Memory
//	***********************
struct memoryUnit{
	char frame[256];
};
int memoryUsed = 0;
struct memoryUnit memory[256];


int getFromBackingStore(char *argv1, int frameNumber, int pageNumber, int offset){
	FILE *pfile;
	pfile = fopen(argv1, "rb");


	fseek(pfile, pageNumber * 256, SEEK_SET);
	
	char buffer[256];
	fread(buffer, sizeof(char), 256, pfile);

	//	*************************
	//		Bring to Memory
	//	*************************

	int frameIndex = memoryUsed;
	for(int i=0;i<256;i++){
		memory[memoryUsed].frame[i] = buffer[i];
	}
	memoryUsed++;

	//	*************************
	//		Update Page Table
	//	*************************

	pageTable[pageNumber].frameNumber 	= frameIndex;
	pageTable[pageNumber].validBit		= 1;

	//	*************************
	//		Update TLB
	//	*************************

	int updateIndex = 0;
	for(int i=0;i<16;i++){
		if(TLB[i].lastTimeUsed > TLB[updateIndex].lastTimeUsed){
			updateIndex = i;
		}
	}
	TLB[updateIndex].pageNumber 	= pageNumber;
	TLB[updateIndex].frameNumber	= frameIndex;
	TLB[updateIndex].haveNum		= 1;
	TLB[updateIndex].lastTimeUsed	= 0;

	for(int i=0;i<16;i++){
		if(i != updateIndex)
			TLB[i].lastTimeUsed++;
	}

	fclose(pfile);
	return frameIndex;
}


//	***********************
//		My Main
//	***********************

int main(int argc,char **argv)
{
	if(argc != 3){
		cout << "Wrong argc...\n";
		exit(0);
	}

	//	***********************
	//		TLB initialize
	//	***********************
	
	for(int i=0;i<16;i++){
		TLB[i].haveNum = 0;
		TLB[i].lastTimeUsed =5000;
	}

	//	***********************
	//	 Page Table Initialize
	//	***********************

	for(int i=0;i<256;i++){
		pageTable[i].validBit = 0;
	}

	//	***********************
	//		Open File
	//	***********************

    ifstream myfile;
    ofstream writeFile;
    myfile.open(argv[2]);
    writeFile.open("results.txt");
    
    int totalRun;
	myfile >> totalRun;
	//cout << totalRun << endl;

	int TLBHIT = 0;
	int PAGEFAULT = 0;
	for(int totalIndex=0;totalIndex<totalRun;totalIndex++){
		
		//	***********************
		//		Get address
		//	***********************

		int address;
		myfile >> address;

		int pageNumber = address >> 8;
		int offset = address - (pageNumber << 8);
		//cout << "pageNumber: " << pageNumber << ", " << "offset: " << offset << endl;
	
		//	***********************
		//		Check TLB
		//	***********************

		int if_TLB_Hit = 0;
		int frameNumber;
		for(int tlbIndex=0;tlbIndex<16;tlbIndex++){
			if(TLB[tlbIndex].haveNum){
				if(pageNumber == TLB[tlbIndex].pageNumber){
					if_TLB_Hit	= 1;

					TLB[tlbIndex].lastTimeUsed = 0;
					for(int i=0;i<16;i++){
						if(i != tlbIndex)
							TLB[i].lastTimeUsed++;
					}

					frameNumber	= TLB[tlbIndex].frameNumber;
					break;
				}
			}
		}

		//	***********************
		//		TLB Hit
		//	***********************

		int physicalIndex;
		if(if_TLB_Hit){
			physicalIndex = (frameNumber << 8) + offset;
			TLBHIT++;
		}

		//	***********************
		//		TLB Miss
		//	***********************

		if(!if_TLB_Hit){
			//cout << "Miss...\n";
			
			//	***********************
			//		Page Table Hit
			//	***********************

			if(pageTable[pageNumber].validBit == 1){
				frameNumber		= pageTable[pageNumber].frameNumber;
				physicalIndex	= (frameNumber << 8) + offset;
			
				//	*******************
				//		Update TLB
				//	*******************
				
				int updateIndex = 0;
				for(int i=0;i<16;i++){
					if(TLB[i].lastTimeUsed > TLB[updateIndex].lastTimeUsed){
						updateIndex = i;
					}
				}
				TLB[updateIndex].pageNumber 	= pageNumber;
				TLB[updateIndex].frameNumber	= frameNumber;
				TLB[updateIndex].haveNum		= 1;
				TLB[updateIndex].lastTimeUsed	= 0;

				for(int i=0;i<16;i++){
					if(i != updateIndex)
						TLB[i].lastTimeUsed++;
				}
			}

			//	***********************
			//		Page Fault
			//	***********************

			if(pageTable[pageNumber].validBit == 0){
				frameNumber = getFromBackingStore(argv[1], frameNumber, pageNumber, offset);
				physicalIndex	= (frameNumber << 8) + offset;
				PAGEFAULT++;
			}
		}

		char value = memory[frameNumber].frame[offset];
		//cout << physicalIndex << " " << (int)value << endl;
		writeFile << physicalIndex << " " << (int)value << endl;
	}

	//cout << "TLB hits: " << TLBHIT << endl;
	//cout << "Page Faults: " << PAGEFAULT << endl;
	writeFile << "TLB hits: " << TLBHIT << endl;
	writeFile << "Page Faults: " << PAGEFAULT << endl;

	writeFile.close();
	myfile.close();

	return 0;
}