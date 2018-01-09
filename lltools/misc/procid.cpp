#include <iostream>

#include "cpuid.h"

int main(int argc, char *argv[]) {
  CPUID cpuid(0x1, 0x0);

  uint32_t familyId = (cpuid.EAX() >> 8) & 0xf;
  if (familyId == 0xf) {
    uint32_t exFamilyId = (cpuid.EAX() >> 20) & 0xff;
    familyId += exFamilyId;
  }

  uint32_t model = (cpuid.EAX() >> 4) & 0xf;
  if ((familyId == 0x6) || (familyId == 0xf)) {
    uint32_t exModel = (cpuid.EAX() >> 16) & 0xf;
    model |= (exModel << 4);
  }

  std::cout << "=========================================" << std::endl;
  std::cout << std::hex << "Family : "
            << "0x" << familyId << ", Model : 0x" << model << std::dec
            << std::endl;
  std::cout << "=========================================" << std::endl;

  return 0;
}
