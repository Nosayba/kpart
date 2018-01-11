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
