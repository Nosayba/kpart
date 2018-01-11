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
