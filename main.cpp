#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <set>
#include <queue>
#include <climits>
#include <string>
#include <memory>
#include "tokenizer.h"

using namespace std;
//------------data structure-------------------
struct process {
    double A;
    double B;
    double C;
    int pageFaultCount; // the number of page faults
    int referenceCount; // the number of reference time
    double residency;   // the residency time
    int evictTime; // the word currently referred
    int nextW; //indicate next word
    process(double A, double B, double C, double residency, int pageFaultCount, int referenceCount, int evictTime)
            :A(A), B(B), C(C), residency(residency), pageFaultCount(pageFaultCount), referenceCount(referenceCount), evictTime(evictTime) {}
};

struct frame {
    int frameNum;
    int processNum;  // indicate process num
    int timestamp;   // indicate the last time the page was used
    int loadedTime;  // indicate the time the page was loaded
    int pageNum;     // indicate page number
    frame(int frameNum, int processNum, int timestamp, int loadedTime)
            :frameNum(frameNum), processNum(processNum), timestamp(timestamp), loadedTime(loadedTime) {}
};



//--------------global variable------------------
const string file = "random-numbers.txt";
tokenizer* randomNumber = new tokenizer(file);
int machineSize;
int pageSize;
int frameCount;
int processSize;
int jobMix;
int referenceCount;
int timeStamp = 1;
int freeFrame;
string pra;

queue<shared_ptr<frame>> fifoQueue;
vector<shared_ptr<frame>> sortedFrame;
vector<shared_ptr<frame>> frameVec;

vector<shared_ptr<process>> processVec;


//--------------Functions-------------------------
//
bool mycmp(shared_ptr<frame> const &f1, shared_ptr<frame> const &f2) {
    return f1->timestamp < f2->timestamp;
}

//read all the input
void readInput(string file) {
    tokenizer* parser = new tokenizer(file);
    machineSize = stoi(parser->getToken());
    pageSize = stoi(parser->getToken());
    processSize = stoi(parser->getToken());
    jobMix = stoi(parser->getToken());
    referenceCount = stoi(parser->getToken());
    pra = parser->getToken();
}

//printout the input
void printInput() {
    printf("The machine size is %d. \n", machineSize);
    printf("The page size is %d. \n", pageSize);
    printf("The process size is %d. \n", processSize);
    printf("The job mix number is %d. \n", jobMix);
    printf("The number of references per process is %d. \n", referenceCount);
    printf("The replacement algorithm is %s. \n", pra.c_str());
    printf("The level of debugging output is 1 \n \n");
}

//initialize all proess and frame
void initialization() {
    // initialize frame
    frameCount = machineSize / pageSize;
    freeFrame = frameCount - 1;
    for (int i = 0; i < frameCount; i++) {
        frameVec.emplace_back(make_shared<frame>(i, 0, 0, 0));
    }

    // initialize process
    switch (jobMix)
    {
        case 1: {

            processVec.emplace_back(make_shared<process>(1.0, 0, 0, 0, 0, 0.0, 0));
            break;
        }
        case 2: {
            for (int i = 0; i < 4; i++) {
                processVec.emplace_back(make_shared<process>(1, 0, 0, 0, 0, 0.0, 0));
            }
            break;
        }
        case 3: {
            for (int i = 0; i < 4; i++) {
                processVec.emplace_back(make_shared<process>(0, 0, 0, 0, 0, 0.0, 0));
            }
            break;
        }
        case 4: {
            processVec.emplace_back(make_shared<process>(0.75, 0.25, 0, 0, 0, 0.0, 0));
            processVec.emplace_back(make_shared<process>(0.75, 0, 0.25, 0, 0, 0.0, 0));
            processVec.emplace_back(make_shared<process>(0.75, 0.125, 0.125, 0, 0, 0.0, 0));
            processVec.emplace_back(make_shared<process>(0.5, 0.125, 0.125, 0, 0, 0.0, 0));
        }
    }
}


//read random number from file
int getRandom() {
    if(randomNumber->nextToken()) {
        return stoi(randomNumber->getToken());
    }
}

void updateFrameAndProcess(shared_ptr<frame> frame0, shared_ptr<process> process0, int processNum, int reference){
    printf("Fault, evicting page %d of process %d from frame %d \n", frame0->pageNum, frame0->processNum, frame0->frameNum);

    // update process
    processVec[processNum - 1]->pageFaultCount++;
    process0->evictTime++;
    process0->residency += (timeStamp - frame0->loadedTime);

    // update frame
    frame0->processNum = processNum;
    frame0->timestamp = timeStamp;
    frame0->loadedTime = timeStamp;
    frame0->pageNum = reference / pageSize;
    timeStamp++;
}

//FIFO algorithmm
void fifoReplace(int processNum, int reference) {
    shared_ptr<frame> frame0 = fifoQueue.front();
    fifoQueue.pop();
    shared_ptr<process> process0 = processVec[frame0->processNum - 1];
    updateFrameAndProcess(frame0, process0, processNum, reference);
    fifoQueue.push(frame0);
}

//LRU algorithn
void lruReplace(int processNum, int reference) {
    sort(sortedFrame.begin(), sortedFrame.end(), mycmp);
    shared_ptr<frame> frame0 = sortedFrame[0];
    shared_ptr<process> process0 = processVec[frame0->processNum - 1];
    updateFrameAndProcess(frame0, process0, processNum, reference);
}

//Random algorithm
void randomReplace(int processNum, int reference) {
    int randomNum = getRandom();
    //printf("  %d uses random number for choosing frame: %d  ", processNum, randomNum);
    shared_ptr<frame> frame0 = frameVec[randomNum % frameCount];
    shared_ptr<process> process0 = processVec[frame0->processNum - 1];
    updateFrameAndProcess(frame0, process0, processNum, reference);
}


//Decide if it's a pagehit or a pagefault
bool hitOrFault(int processNum, int reference) {
    //Hit page
    for(int i = 0; i < frameVec.size(); i++) {
        if (frameVec[i]->processNum == processNum && frameVec[i]->pageNum == reference / pageSize) {
            frameVec[i]->timestamp = timeStamp;
            printf("Hit in frame %d \n", i);
            timeStamp++;
            return true;
        }
    }
    //Page fault
    if (freeFrame >= 0) {
        shared_ptr<frame> frame0 = frameVec[freeFrame];
        frame0->processNum = processNum;
        frame0->timestamp = timeStamp;
        frame0->loadedTime = timeStamp;
        frame0->pageNum = reference / pageSize;
        if (pra == "lru") {
            sortedFrame.push_back(frame0);
        } else if (pra == "fifo") {
            fifoQueue.push(frame0);
            cout<<" size "<<fifoQueue.size();
        }
        printf("Fault: using free frame %d \n", freeFrame);
        processVec[processNum - 1]->pageFaultCount++;
        freeFrame--;
        timeStamp++;
    } else if (pra == "lru") {
        lruReplace(processNum, reference);
    } else if (pra == "fifo") {
        fifoReplace(processNum, reference);
    } else {
        randomReplace(processNum, reference);
    }
    return false;
}

//calculate next reference for process
int nextReference(int i) {
    double A = processVec[i]->A, B = processVec[i]->B, C = processVec[i]->C;
    int w = processVec[i]->nextW, nextW;
    if (processVec[i]->referenceCount == 0) {
        w = (111 * (i + 1)) % processSize;
    }
    printf("%d references word %d (page %d) at time %d:", i+1, w, w / pageSize, timeStamp);
    hitOrFault(i + 1, w);
    int randomNum1 = getRandom(), randomNum2 = 0;


    double y = randomNum1 / (INT_MAX + 1.0);
    if (y < A) {
        nextW =  (w + 1) % processSize;
    } else if (y < A + B) {
        nextW =  (w - 5 + processSize) % processSize;
    } else if (y < A + B + C) {
        nextW = (w + 4) % processSize;
    } else {
        randomNum2 = getRandom();
        nextW = randomNum2 % processSize;
    }
    processVec[i]->nextW = nextW;
    //printf("%d uses random number: %d\n", i+1, randomNum1);
    //if(randomNum2 != 0) printf("%d uses random number: %d\n", i+1, randomNum2);
    processVec[i]->referenceCount++;
    return nextW;
}

bool finished() {
    for(int i = 0; i < processVec.size(); i++) {
        if (processVec[i]->referenceCount < referenceCount) {
            return false;
        }
    }
    return true;
}

void printOutput() {
    cout << endl;
    int total = 0;
    double residency = 0, evictTime = 0;
    for (int i = 0; i < processVec.size(); i++) {
        total += processVec[i]->pageFaultCount;
        residency += processVec[i]->residency;
        evictTime += processVec[i]->evictTime;
        if (processVec[i]->residency != 0) {
            printf("Process %d had %d faults and %f average residency \n", i + 1, processVec[i]->pageFaultCount,
                   processVec[i]->residency / processVec[i]->evictTime);
        } else {
            printf("Process %d had %d faults. \n", i+1, processVec[i]->pageFaultCount);
            printf("     With no evictions, the average residence is undefined.\n");
        }
    }
    if (residency != 0) {
        printf("\nThe total number of faults is %d and the overall average residency is %f.\n", total, residency / evictTime);
    } else {
        printf("\nThe total number of faults is %d\n", total);
        printf("     With no evictions, the average residence is undefined.\n");
    }
}

int main(int argc, char* argv[]) {
    string filename(argv[1]);
    readInput(filename);
    //readInput("example10.txt");
    printInput();
    initialization();
    while(!finished()) {
        for (int i = 0; i < processVec.size(); i++) {
            for (int j = 0; j < 3 && processVec[i]->referenceCount < referenceCount; j++) {
                nextReference(i);
            }
        }
    }
    printOutput();
    return 0;
}