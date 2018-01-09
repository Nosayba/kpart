#include <unistd.h>

#include <iostream>
#include <limits>
#include <vector>

#include "cmt.h"
#include "sysconfig.h"

using namespace std;

void usage(char *argv[]) {
  cout << "USAGE:" << endl;
  cout << argv[0] << " [-g] [-r rmid] [-a] [-c core] [-h]" << endl;
  cout << "\t-c core: Perform action for the specified core" << endl;
  cout << "\t-a: Perform action for all cores in the systems" << endl;
  cout << "\t-g: Get RMID for specified cores" << endl;
  cout << "\t-r rmid: Set resource management ID (RMID) to specified value for "
       << "specified cores" << endl;
  cout << "\t-h: Print this help" << endl;
  cout << "NOTE: Needs sudo to run" << endl;
}

int main(int argc, char *argv[]) {
  int c;
  const uint32_t invalid_rmid = numeric_limits<uint32_t>::max();
  uint32_t rmid = invalid_rmid;
  bool set = false;
  vector<int> cores;

  while ((c = getopt(argc, argv, "ac:r:gh")) != -1) {
    switch (c) {
    case 'g':
      set = false;
      break;
    case 'c':
      cores.resize(0);
      cores.push_back(atoi(optarg));
      break;
    case 'a':
      cores.resize(0);
      for (int c = 0; c < getNumCores(); ++c) {
        cores.push_back(c);
      }
      break;
    case 'r':
      set = true;
      rmid = atoi(optarg);
      break;
    case 'h':
      usage(argv);
      return 0;
      break;
    case '?':
      usage(argv);
      return -1;
      break;
    }
  }

  if (cores.size() == 0) {
    usage(argv);
    return -1;
  }

  CMTController cmt(set);

  if (set) {
    if (rmid == invalid_rmid) {
      usage(argv);
      return -1;
    }

    for (int core : cores) {
      cmt.setRmid(core, rmid);
    }
  }

  for (int core : cores) {
    cout << "Core " << core << " : RMID " << cmt.getRmid(core) << endl;
  }

  return 0;
}
