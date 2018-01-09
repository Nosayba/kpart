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
