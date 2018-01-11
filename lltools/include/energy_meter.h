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

#include <math.h>
#include "msr.h"
#include "msr_haswell.h"

class EnergyMeter {
private:
  MSR msr;
  double energyUnit;
  double powerUnit;
  uint64_t energyMask;

  double t0PkgEnergy;
  double t0PP0Energy;

  double cumEnergy(uint64_t msrNo) const {
    uint64_t msrVal = msr.read(0, msrNo);
    double energy = (msrVal & energyMask) * energyUnit;
    return energy;
  }

public:
  EnergyMeter() {
    uint64_t mask;
    uint64_t msrPowerUnit = msr.read(0, MSR_RAPL_POWER_UNIT);

    mask = 0x1F << 8;                               // bits[12:8]
    uint64_t exponent = (msrPowerUnit & mask) >> 8; // bits[12:8]
    energyUnit = 1.0 / pow(2.0, exponent);

    mask = 0xF;                     // bits[3:0]
    exponent = msrPowerUnit & mask; // bits[3:0]
    powerUnit = 1.0 / pow(2.0, exponent);

    energyMask = 0xFFFFFFFF; // bits[31:0]
  }

  double getTdp() const {
    uint64_t msrPowerInfo = msr.read(0, MSR_PKG_POWER_INFO);
    uint64_t mask = 0x7FFFL; // bits[14:0]
    double tdp = (msrPowerInfo & mask) * powerUnit;
    return tdp;
  }

  double getMaxPower() const {
    uint64_t msrPowerInfo = msr.read(0, MSR_PKG_POWER_INFO);
    uint64_t mask = 0x7FFFL << 32; // bits[46:32]
    double maxPower = (msrPowerInfo & mask) >> 32;
    maxPower *= powerUnit;
    return maxPower;
  }

  double getMinPower() const {
    uint64_t msrPowerInfo = msr.read(0, MSR_PKG_POWER_INFO);
    uint64_t mask = 0x7FFFL << 16; // bits[30:16]
    double minPower = (msrPowerInfo & mask) >> 16;
    minPower *= powerUnit;
    return minPower;
  }

  double getPkgEnergy() const {
    double pkgEnergy = cumEnergy(MSR_PKG_ENERGY_STATUS);
    if (pkgEnergy < t0PkgEnergy) { // overflow
      pkgEnergy += (energyMask + 1);
    }
    return pkgEnergy - t0PkgEnergy;
  }

  double getPP0Energy() const {
    double pp0Energy = cumEnergy(MSR_PP0_ENERGY_STATUS);
    if (pp0Energy < t0PP0Energy) { // overflow
      pp0Energy += (energyMask + 1);
    }
    return pp0Energy - t0PP0Energy;
  }

  void clear() {
    t0PkgEnergy = cumEnergy(MSR_PKG_ENERGY_STATUS);
    t0PP0Energy = cumEnergy(MSR_PP0_ENERGY_STATUS);
  }
};
