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

#include <exception>
#include <sstream>
#include <string>

#include "cpuid.h"
#include "msr.h"
#include "msr_haswell.h"
#include "sysconfig.h"

/*******************************************************************************
 * CPUID specs obtain from Intel SDM Vol 3 (Sys. Prog. Guide) Chapter 17
 *******************************************************************************/

class CATException : public std::exception {
private:
  std::string error;

public:
  CATException(std::string error) : error(error) {}
  ;

  const char *what() const throw() { return error.c_str(); }
};

class CATController {
private:
  MSR msr;
  int numCores;
  int cbmLen;
  int numCos;
  bool cdpEnabled;

public:
  CATController(bool write = true) : msr(write) {
    numCores = getNumCores();
    assert(numCores > 0);

    // Ensure CAT is supported
    if (!catSupported()) {
      throw CATException("CAT not present");
    }

    if (!l3CatSupported()) {
      throw CATException("L3 CAT not present");
    }

    cbmLen = getCbmLen();
    numCos = getNumCos();
    cdpEnabled = getCdpStatus();
  }

  static bool catSupported() {
    CPUID catCpuId(0x7, 0x0);
    const uint32_t CAT_BIT = 12; //bit 12 instead of bit 15
    return ((catCpuId.EBX() & (1 << CAT_BIT)) >> CAT_BIT) != 0;
  }

  static bool l3CatSupported() {
    const uint32_t L3_BIT = 1;
    CPUID l3CatCpuId(0x10, 0x0);
    return ((l3CatCpuId.EBX() & (1 << L3_BIT)) >> L3_BIT) != 0;
  }

  static int getNumCos() {
    const uint32_t numCosMask = 0xFFFF;
    CPUID resCpuId(0x10, 0x1);

    // std::cout << resCpuId.EAX() << std::endl; // 0
    // std::cout << resCpuId.EBX()  << std::endl;  // 49152
    // std::cout << resCpuId.ECX()  << std::endl;  // 47
    // std::cout << resCpuId.EDX()  << std::endl;  // 1

    // resCpuId(0x10, 0x1) => 1
    //resCpuId(0x0, 0x0) => 2

    return (resCpuId.EDX() & numCosMask) + 1;
    //return 4;
  }

  static int getCbmLen() {
    const uint32_t cbmLenMask = 0x1F;
    CPUID resCpuId(0x10, 0x1);
    return (resCpuId.EAX() & cbmLenMask) + 1; //returns 1
                                              //return 20;
  }

  static bool getCdpStatus() {
    const uint32_t cdpMask = 0x2;
    CPUID resCpuId(0x10, 0x1);
    return ((resCpuId.ECX() & cdpMask) > 0);
  }

  uint32_t getCos(int core) {
    uint64_t cos = msr.read(core, MSR_IA32_PQR_ASSOC);
    cos >>= 32;
    return static_cast<uint32_t>(cos);
  }

  void setCos(int core, uint32_t cos) {
    if (cos >= numCos) {
      std::stringstream ss;
      ss << "cos (" << cos << ") exceeds max supported cos (" << numCos - 1
         << ")";
      throw CATException(ss.str());
    }

    assert(cos < numCos);
    uint64_t pqrAssoc = msr.read(core, MSR_IA32_PQR_ASSOC);
    pqrAssoc &= 0xFFFFFFFF; // clear away cos
    uint64_t cosBits = cos;
    cosBits <<= 32;
    pqrAssoc |= cosBits;
    msr.write(core, MSR_IA32_PQR_ASSOC, pqrAssoc);
  }

  void setGlobalCos(int cos) {
    for (int i = 0; i < numCores; i++)
      setCos(i, cos);
  }

  uint32_t getCbm(int cos) {
    if (cos >= numCos) {
      std::stringstream ss;
      ss << "cos (" << cos << ") exceeds max supported cos (" << numCos - 1
         << ")";
      throw CATException(ss.str());
    }
    uint64_t l3Mask = msr.read(0, MSR_IA32_L3_MASK_0 + cos);
    l3Mask &= 0xFFFFFFFF; // bits 31:0
    return static_cast<uint32_t>(l3Mask);
  }

  void setCbm(int cos, uint32_t cbm) {
    if (cos >= numCos) {
      std::stringstream ss;
      ss << "cos (" << cos << ") exceeds max supported cos (" << numCos - 1
         << ")";
      throw CATException(ss.str());
    } else if (cbm >> cbmLen != 0) {
      std::stringstream ss;
      ss << "Length of capacity bit mask exceeds max value (" << cbmLen << ")";
      throw CATException(ss.str());
    }

    uint64_t l3Mask = msr.read(0, MSR_IA32_L3_MASK_0 + cos);
    l3Mask &= 0xFFFFFFFF00000000; // clear away bit mask
    uint64_t cbmBits = cbm;
    l3Mask |= cbmBits;
    msr.write(0, MSR_IA32_L3_MASK_0 + cos, l3Mask);
  }
};
