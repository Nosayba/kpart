/** $lic$
* MIT License
* 
* Copyright (c) 2017-2018 by Massachusetts Institute of Technology
* Copyright (c) 2017-2018 by Qatar Computing Research Institute, HBKU
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* If you use this software in your research, we request that you reference
* the KPart paper ("KPart: A Hybrid Cache Partitioning-Sharing Technique for
* Commodity Multicores", El-Sayed, Mukkara, Tsai, Kasture, Ma, and Sanchez,
* HPCA-24, February 2018) as the source in any publications that use this
* software, and that you send us a citation of your work.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
**/
#include <unistd.h>

#include <iostream>
#include <limits>
#include <vector>

#include "cat.h"
#include "sysconfig.h"

using namespace std;

void usage(char *argv[]) {
  cout << "USAGE:" << endl;
  cout << argv[0] << " [-g] [-s cos] [-a] [-c core] [-h]" << endl;
  cout << "\t-c core: Perform action for the specified core" << endl;
  cout << "\t-a: Perform action for all cores in the systems" << endl;
  cout << "\t-g: Get COS for specified cores" << endl;
  cout << "\t-s cos: Set class of service (COS) to specified value for "
       << "specified cores" << endl;
  cout << "\t-h: Print this help" << endl;
  cout << "NOTE: Needs sudo to run" << endl;
}

int main(int argc, char *argv[]) {

  int c;
  bool set = false;
  vector<int> cores;
  cores.resize(0);
  const uint32_t invalid_cos = numeric_limits<uint32_t>::max();
  uint32_t cos = invalid_cos;

  while ((c = getopt(argc, argv, "gs:ac:h")) != -1) {
    switch (c) {
    case 'g':
      set = false;
      break;
    case 's':
      set = true;
      cos = atoi(optarg);
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

  CATController ctrl(set);

  if (set) {
    for (int c : cores) {
      ctrl.setCos(c, cos);
    }
  } else {
    for (int c : cores) {
      cout << "Core " << c << " : COS " << ctrl.getCos(c) << endl;
    }
  }

  return 0;
}
