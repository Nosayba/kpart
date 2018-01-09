#include <iostream>
#include <numeric>
#include "timetools.h"
#include <vector>

using namespace std;

// int vals[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
// int res[] = {0, 0, 0, 0, 0, 0, 0, 0};
// void process() {
//     uint64_t count;
//     const int64_t loopcnt = 10000;
//     for (count = 0; count < loopcnt; count++) {
//         res[0] = vals[0] + vals[1];
//         res[1] = vals[2] * vals[3];
//         res[2] = vals[4] * vals[5];
//         res[3] = vals[6] * vals[7];
//         res[4] = vals[8] * vals[9];
//         res[5] = vals[10] * vals[11];
//         res[6] = vals[12] * vals[13];
//         res[7] = vals[14] * vals[15];
//     }
// }

void process() {
  int64_t loopcnt = 10000;
  int val = 0;
  asm volatile("1:;"
               "add $1, %0;"
               "add $1, %0;"
               "add $1, %0;"
               "add $1, %0;"
               "add $1, %0;"
               "add $1, %0;"
               "add $1, %0;"
               "add $1, %0;"
               "add $1, %0;"
               "add $1, %0;"
               "add $1, %0;"
               "add $1, %0;"
               "add $1, %0;"
               "add $1, %0;"
               "add $1, %0;"
               "add $1, %0;"
               "sub $1, %1;"
               "jnz 1b;"
               : "=r"(val), "=r"(loopcnt)
               : "0"(val), "1"(loopcnt));
}

int main(int argc, char *argv[]) {
  vector<uint64_t> procTimes;
  uint64_t curNs = getCurNs();
  for (int i = 0; i < 10; i++) {
    process();
    uint64_t ns = getCurNs();
    procTimes.push_back(ns - curNs);
    curNs = ns;
  }

  float avg = accumulate(procTimes.begin(), procTimes.end(), 0);
  avg /= procTimes.size();
  avg /= 1e6;
  cout << "time = " << avg << " ms" << endl;
  return 0;
}
