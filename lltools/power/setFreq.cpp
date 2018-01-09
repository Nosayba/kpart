#include <algorithm>
#include <assert.h>
#include "dvfs.h"
#include <stdlib.h>
#include "sysconfig.h"
#include <unistd.h>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
  int numCores = getNumCores();
  vector<int> freqs = getFreqsMHz();
  vector<int> cores;
  int freq = -1;

  int c;
  while ((c = getopt(argc, argv, "ac:f:")) != -1) {
    switch (c) {
    case 'a':
      cores.clear();
      for (int c = 0; c < numCores; c++)
        cores.push_back(c);
      break;
    case 'c':
      cores.clear();
      cores.push_back(atoi(optarg));
      assert(cores.back() < numCores);
      break;
    case 'f':
      freq = atoi(optarg);
      break;
    }
  }

  assert(cores.size() > 0);
  assert(find(freqs.begin(), freqs.end(), freq) != freqs.end());

  VFController ctrl;
  for (int c : cores)
    ctrl.setFreq(c, freq);

  return 0;
}
