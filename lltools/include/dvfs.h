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
#pragma once

#include <assert.h>
#include "msr.h"
#include "msr_haswell.h"
#include "sysconfig.h"

// MSR_IA32_PERF_CTL[15:8] encodes requested frequency in units of
// 10^8 Hz
// MSR_IA32_PERF_STATUS encodes current frequency in a similar manner
class VFController {
private:
  MSR msr;
  int numCores;

public:
  VFController() {
    numCores = getNumCores();
    assert(numCores > 0);
  }

  ~VFController() {}

  void setFreq(int core, int freqMHz) {
    uint64_t freq = (freqMHz / 100) << 8;
    msr.write(core, MSR_IA32_PERF_CTL, freq);
  }

  void setGlobalFreq(int freqMHz) {
    for (int c = 0; c < numCores; c++) {
      setFreq(c, freqMHz);
    }
  }

  int getFreq(int core) {
    uint64_t perfStatus = msr.read(core, MSR_IA32_PERF_STATUS);
    int freqMHz = ((perfStatus & 0xFFFF) >> 8) * 100;
    return freqMHz;
  }

  int getFreqTgt(int core) {
    uint64_t perfTgt = msr.read(core, MSR_IA32_PERF_CTL);
    int freqMHz = ((perfTgt & 0xFFFF) >> 8) * 100;
    return freqMHz;
  }
};
