#include <cstdlib>
#include <iostream>

#include "msr.h"

void usage(char *argv[]) {
  std::cerr << std::endl;
  std::cerr << "Usage: " << argv[0] << " [-e] [-d] [-h]" << std::endl;
  std::cerr << "  -e : Enable all prefetchers" << std::endl;
  std::cerr << "  -d : Disable all prefetchers" << std::endl;
  std::cerr << "  -h : Print this help" << std::endl;
  std::cerr << std::endl;
}

int main(int argc, char *argv[]) {
  int numCores = getNumCores();
  bool enable = false;

  if (argc != 2) {
    usage(argv);
    return -1;
  }

  int c;
  while ((c = getopt(argc, argv, "edh")) != -1) {
    switch (c) {
    case 'e':
      enable = true;
      break;
    case 'd':
      enable = false;
      break;
    case 'h':
      usage(argv);
      return 0;
    case '?':
      std::cerr << "Invalid argument : " << optarg << std::endl;
      usage(argv);
      return -1;
      break;
    }
  }

  MSR msr;
  const ssize_t PREFETCH_MSR = 0x1a4;
  uint64_t val = enable ? 0x0 : 0xF; // 0x0 enables all, 0xF disables all

  for (int c = 0; c < numCores; ++c) {
    msr.write(c, PREFETCH_MSR, val);
  }

  return 0;
}
