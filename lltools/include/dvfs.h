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
