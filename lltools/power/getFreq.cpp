#include <assert.h>
#include "dvfs.h"
#include <iostream>
#include <stdlib.h>
#include "sysconfig.h"
#include <unistd.h>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
  int numCores = getNumCores();
  vector<int> freqs = getFreqsMHz();
  vector<int> cores;

  int c;
  while ((c = getopt(argc, argv, "ac:")) != -1) {
    switch (c) {
    case 'a':
      cores.clear();
      for (int c = 0; c < numCores; c++)
        cores.push_back(c);
      break;
    case 'c':
      int core = atoi(optarg);
      assert(core < numCores);
      cores.clear();
      cores.push_back(core);
      break;
    }
  }

  assert(cores.size() > 0);

  VFController ctrl;
  for (int c : cores) {
    cout << "Core " << c << " | Freq Target = " << ctrl.getFreqTgt(c)
         << " MHz | Freq = " << ctrl.getFreq(c) << " MHz" << endl;
  }

  return 0;
}
