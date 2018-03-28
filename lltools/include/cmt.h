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
#include <iostream>
#include "cpuid.h"
#include "msr.h"
#include "msr_haswell.h"
#include "sysconfig.h"

class CMTException : public std::exception {
private:
  std::string error;

public:
  CMTException(std::string error) : error(error) {}
  ;

  const char *what() const throw() { return error.c_str(); }
};

class CMTController {
private:
  MSR msr;

  // CMT events
  const uint32_t LLC_OCCUPANCY = 0x1;
  const uint32_t TOTAL_MEM_BW = 0x2;
  const uint32_t LOCAL_MEM_BW = 0x3;
  //const uint32_t TOTAL_MEM_BW = 0x4;
  //const uint32_t LOCAL_MEM_BW = 0x2;

  // CMT event multiplier to convert IA32_QM_CTR counts into bytes
  int64_t cmt_multiplier;

  // Always read QoS Monitoring events for core 0. This works fine on
  // blb3 which only has a single physical package. On multi-socket
  // systems, one needs to figure out which package to read data for.
  const int CORE = 0;

  void setEvtselRmid(uint64_t rmid) {
    const uint64_t mask = 0x3ff; // 10 bits
    const uint32_t shift = 32;

    rmid &= mask;
    rmid <<= shift;

    uint64_t evtsel = msr.read(CORE, MSR_IA32_QM_EVTSEL);
    evtsel &= ~(mask << shift);
    evtsel |= rmid;

    msr.write(CORE, MSR_IA32_QM_EVTSEL, evtsel);
  }

  void setEvtselEvt(uint64_t evt) {
    const uint64_t mask = 0xff; // 8 bits

    evt &= mask;

    uint64_t evtsel = msr.read(CORE, MSR_IA32_QM_EVTSEL);
    evtsel &= ~mask;
    evtsel |= evt;

    msr.write(CORE, MSR_IA32_QM_EVTSEL, evtsel);
  }

  int64_t readQmCtr(uint64_t rmid, uint64_t evt) {
    //rmid = 0;  //TEST
    setEvtselRmid(rmid);
    setEvtselEvt(evt);

    int64_t ctr = msr.read(CORE, MSR_IA32_QM_CTR);

    //std::cout << "rmid = " <<  rmid << std::endl;
    //std::cout << "evt = " <<  evt << std::endl;
    //std::cout << "ctr = " <<  ctr << std::endl;

    if (ctr & (0x1ULL << 63)) { // bad rmid or evt
      throw CMTException("Baaaaaad RMID or Event Type");
      return -1;
    }

    if (ctr & (0x1ULL << 62)) { // ctr unavailable
      throw CMTException("Counter unavailable");
      return -1;
    }

    ctr *= cmt_multiplier;

    return ctr;
  }

public:
  CMTController(bool write = true) : msr(write) {
    CPUID cmtMulCpuId(0xf, 0x1);
    cmt_multiplier = cmtMulCpuId.EBX();
  }

  ~CMTController() {}

  void setRmid(int core, uint64_t rmid) {
    uint64_t mask = 0x3ff; // 10 bits
    rmid &= mask;

    uint64_t pqr_assoc = msr.read(core, MSR_IA32_PQR_ASSOC);
    pqr_assoc &= ~mask;
    pqr_assoc |= rmid;

    msr.write(core, MSR_IA32_PQR_ASSOC, pqr_assoc);
  }

  void setGlobalRmid(uint64_t rmid) {
    for (int c = 0; c < getNumCores(); ++c) {
      setRmid(c, rmid);
    }
  }

  uint64_t getRmid(int core) {
    uint64_t mask = 0x3ff; // 10 bits
    uint64_t pqr_assoc = msr.read(core, MSR_IA32_PQR_ASSOC);
    return (pqr_assoc & 0x3ff);
  }

  uint64_t getGlobalRmid() {
    uint64_t rmid = getRmid(CORE);
    for (int c = 0; c < getNumCores(); ++c) {
      if (getRmid(c) != rmid) {
        throw CMTException("No global RMID");
      }
    }

    return rmid;
  }

  int64_t getLlcOccupancy(uint64_t rmid) {
    return readQmCtr(rmid, LLC_OCCUPANCY);
  }

  int64_t getTotalMemTraffic(uint64_t rmid) {
    return readQmCtr(rmid, TOTAL_MEM_BW);
  }

  int64_t getLocalMemTraffic(uint64_t rmid) {
    return readQmCtr(rmid, LOCAL_MEM_BW);
  }

  // This method returns the upper limit of DRAM traffic, measured in bytes,
  // that can be reported from CMT before the counter overflows. In current
  // implementations, the width of IA32_QM_CTR.data is 24-bits; this is
  // hardcoded below since Intel does not provide a way to determine this
  // programatically. This is then multiplied with the appropriate multiplier 
  // (CPUID.(EAX=0FH, ECX=1H).EBX) to get the actual memory traffic in bytes.
  // See Intel® 64 and IA-32 Architectures Software Developer’s Manual Volume
  // 3B: System Programming Guide, Part 2, Chapter 17 for more details.
  int64_t getMemTrafficMax() const {
      return cmt_multiplier * (1LL << 24);
  }
};
