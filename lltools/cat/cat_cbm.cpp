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
#include <string>
#include <vector>

#include "cat.h"

using namespace std;

void parseCbm(vector<int> &cbm_entries, string str) {
  size_t remaining_len = str.size();
  size_t begin = 0;
  size_t pos = 0;
  string delimiter = ",";

  while ((pos = str.find_first_of(delimiter, begin)) != string::npos) {
    size_t len = pos - begin;
    cbm_entries.push_back(stoi(str.substr(begin, len)));
    begin = pos + 1;
    remaining_len -= len;
  }

  if (remaining_len > 0) {
    cbm_entries.push_back(stoi(str.substr(begin, remaining_len)));
  }
}

uint32_t getCbm(const vector<int> &cbm_entries) {
  uint32_t cbm = 0;
  for (uint32_t c : cbm_entries) {
    cbm |= 1U << c;
  }
  return cbm;
}

vector<int> getCbmEntries(uint32_t cbm) {
  vector<int> entries;
  int core = 0;
  while (cbm > 0) {
    int present = cbm & 0x1U;
    if (present)
      entries.push_back(core);
    ++core;
    cbm >>= 1;
  }

  return entries;
}

void usage(char *argv[]) {
  cout << "USAGE:" << endl;
  cout << argv[0] << " [-g] [-m cbm_entries] [-c cos] [-h]" << endl;
  cout << "\t-c cos: Perform action for the specified class of "
       << "service (COS)" << endl;
  cout << "\t-g: Get capacity bit mask (CBM) entries for specified COS" << endl;
  cout << "\t-m cbm comma-sep-list: Set CBM for the specified COS to "
       << " include specified entries " << endl;
  cout << "\t-h: Print this help" << endl;
  cout << "NOTE: Needs sudo to run" << endl;
}

int main(int argc, char *argv[]) {
  int c;
  const uint32_t invalid_cos = numeric_limits<uint32_t>::max();
  uint32_t cos = invalid_cos;
  bool set = false;
  vector<int> cbm_entries;

  while ((c = getopt(argc, argv, "gc:m:h")) != -1) {
    switch (c) {
    case 'g':
      set = false;
      break;
    case 'c':
      cos = atoi(optarg);
      break;
    case 'm':
      set = true;
      parseCbm(cbm_entries, optarg);
      break;
    case 'h':
      usage(argv);
      return 0;
    case '?':
      usage(argv);
      return -1;
    }
  }

  if (cos == invalid_cos) {
    usage(argv);
    return -1;
  }

  CATController ctrl(set);

  if (set) {
    uint32_t cbm = getCbm(cbm_entries);
    ctrl.setCbm(cos, cbm);
  } else {
    cbm_entries = getCbmEntries(ctrl.getCbm(cos));
    cout << "CBM entries for COS " << cos << ": ";
    for (int e : cbm_entries)
      cout << e << " ";
    cout << endl;
  }

  return 0;
}
