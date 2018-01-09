#include <iostream>
#include <string>
#include "cpuid.h"

using namespace std;

int main(int argc, char *argv[]) {
  auto present = [](uint32_t presentInt) {
    return (presentInt > 0) ? "PRESENT" : "NOT PRESENT";
  }
  ;
  uint32_t CMT_BIT = 12;
  CPUID cmtCpuId(0x7, 0x0); // Is CMT enabled?
  uint32_t cmtMask = 1 << CMT_BIT;
  uint32_t cmt = (cmtCpuId.EBX() & cmtMask) >> CMT_BIT;
  cout << "=========================================" << endl;
  cout << "CMT : " << present(cmt) << endl;

  if (cmt > 0) {
    uint32_t L3_BIT = 1;
    CPUID l3CpuId(0xf, 0x0); // is L3 CMT supported?
    uint32_t l3Mask = 1 << L3_BIT;
    uint32_t l3Cmt = (l3CpuId.EDX() & l3Mask) >> L3_BIT;
    cout << "l3 CMT = " << present(l3Cmt)
         << ", global max RMID = " << l3CpuId.EBX() << endl;

    if (l3Cmt > 0) {
      CPUID resCpuId(0xf, 0x1);
      uint32_t scalingFactor = resCpuId.EBX();
      uint32_t maxRmid = resCpuId.ECX();
      uint32_t supportedRes = resCpuId.EDX();
      uint32_t l3OccupBit = 0;
      uint32_t totalMemBwBit = 1;
      uint32_t localMemBwBit = 2;
      uint32_t l3Occup = (supportedRes & (1 << l3OccupBit)) >> l3OccupBit;
      uint32_t totalMemBw =
          (supportedRes & (1 << totalMemBwBit)) >> totalMemBwBit;
      uint32_t localMemBw =
          (supportedRes & (1 << localMemBwBit)) >> localMemBwBit;
      cout << "L3 Occpuancy monitoring : " << present(l3Occup) << endl;
      cout << "Total Mem BW monitoring : " << present(totalMemBw) << endl;
      cout << "Local Mem BW monitoring : " << present(localMemBw) << endl;
      cout << "max RMID = " << maxRmid << endl;
      cout << "Scaling factor = " << scalingFactor << endl;
    }
  }

  cout << "=========================================" << endl;
  return 0;
}
