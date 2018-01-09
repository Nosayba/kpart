#include <array>
#include "dvfs.h"
#include <iostream>
#include <math.h>
#include "msr.h"
#include <sstream>
#include "sysconfig.h"
#include "timetools.h"
#include <unistd.h>
#include <vector>

using namespace std;

void process() {
  int64_t loopcnt = 1000;
  int val1 = 0;
  int val2 = 0;
  int val3 = 0;
  int val4 = 0;
  asm volatile("1:;"
               "add $1, %0;"
               "add $1, %1;"
               "add $1, %2;"
               "add $1, %3;"
               "add $1, %0;"
               "add $1, %1;"
               "add $1, %2;"
               "add $1, %3;"
               "add $1, %0;"
               "add $1, %1;"
               "add $1, %2;"
               "add $1, %3;"
               "add $1, %0;"
               "add $1, %1;"
               "add $1, %2;"
               "add $1, %3;"
               "sub $1, %4;"
               "jnz 1b;"
               : "=r"(val1), "=r"(val2), "=r"(val3), "=r"(val4), "=r"(loopcnt)
               : "0"(val1), "1"(val2), "2"(val3), "3"(val4), "4"(loopcnt));
}

vector<int> reversed(vector<int> &vec) {
  vector<int> ret;
  for (auto it = vec.rbegin(); it != vec.rend(); it++) {
    ret.push_back(*it);
  }
  return ret;
}

vector<int> randomChoice(vector<int> &vec, int size) {
  srand(0xdeadbeef);
  vector<int> ret;
  int prevIdx = -1;
  while (ret.size() < size) {
    int idx = rand() % (vec.size() - 1);
    if (idx == prevIdx)
      continue;
    ret.push_back(vec[idx]);
    prevIdx = idx;
  }
  return ret;
}

vector<int> extremeUp(vector<int> &vec) {
  vector<int> ret;
  ret.push_back(vec.front());
  ret.push_back(vec.back());
  return ret;
}

vector<int> extremeDown(vector<int> &vec) {
  vector<int> ret;
  ret.push_back(vec.back());
  ret.push_back(vec.front());
  return ret;
}

int main(int argc, char *argv[]) {
  vector<int> freqs = getFreqsMHz();

  string seqStr = "up";
  int c;
  bool verbose = false;
  while ((c = getopt(argc, argv, "rudEev")) != -1) {
    switch (c) {
    case 'r':
      seqStr = "random";
      freqs = randomChoice(freqs, freqs.size());
      break;
    case 'u':
      break;
    case 'd':
      seqStr = "down";
      freqs = reversed(freqs);
      break;
    case 'E':
      seqStr = "extreme-up";
      freqs = extremeUp(freqs);
      break;
    case 'e':
      seqStr = "extreme-down";
      freqs = extremeDown(freqs);
      break;
    case 'v':
      verbose = true;
      break;
    }
  }

  cout << "Stepping through frequencies in " << seqStr << " order" << endl;

  vector<vector<uint64_t> > procTimes;
  procTimes.resize(freqs.size());
  const int numIters = 1000;
  VFController ctrl;
  for (int i = 0; i < freqs.size(); i++) {
    int freq = freqs[i];
    uint64_t begin = getCurNs();
    ctrl.setGlobalFreq(freq);
    for (int j = 0; j < numIters; j++) {
      process();
      uint64_t end = getCurNs();
      procTimes[i].push_back(end - begin);
      begin = end;
    }
  }

  int prevFreq = freqs[0];
  for (int i = 1; i < freqs.size(); i++) {
    int freq = freqs[i];
    float ssTime = 0.0;
    int ssStart = procTimes[i].size() / 2;
    int ssEnd = procTimes[i].size();

    for (int t = ssStart; t < ssEnd; t++) {
      ssTime += procTimes[i][t];
    }

    ssTime /= (ssEnd - ssStart);

    int transientEnd = 0;
    for (; transientEnd < ssStart; transientEnd++) {
      if (fabs(procTimes[i][transientEnd] - ssTime) < 0.02 * ssTime)
        break;
    }

    if (verbose) {
      cout << "ssStart = " << ssStart << " | ssEnd = " << ssEnd
           << " | ssTime = " << ssTime << "| last = " << procTimes[i][ssEnd - 1]
           << " | transientEnd = " << transientEnd
           << " | lastTrans = " << procTimes[i][transientEnd] << endl;
    }

    uint64_t transientTime = 0;
    for (int j = 0; j <= transientEnd; j++)
      transientTime += procTimes[i][j];

    cout << "Transition " << prevFreq << " --> " << freq << " | "
         << transientTime << " ns" << endl;

    prevFreq = freq;
  }

  if (verbose) {
    for (int i = 0; i < freqs.size(); i++) {
      int freq = freqs[i];
      cerr << "======================================" << endl;
      cerr << "Freq: " << freq << " MHz | ";
      for (int j = 0; j < numIters; j++)
        cerr << procTimes[i][j] << " ";
      cerr << endl;
      cerr << "======================================" << endl << endl;
    }
  }

  return 0;
}
