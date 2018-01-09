#pragma once

#include <algorithm>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

inline int getNumCores() {
  // Unimplemented for gcc versions < 4.8
  // int numCores = std::thread::hardware_concurrency();
  int numCores = sysconf(_SC_NPROCESSORS_ONLN);
  return (numCores > 0) ? numCores : -1;
}

static std::vector<int> getFreqsMHz() {
  std::vector<int> freqs;
  const int BUFSIZE = 1024;
  char buf[BUFSIZE];
  std::ifstream fd("/sys/devices/system/cpu/cpu0/cpufreq/"
                   "scaling_available_frequencies");
  fd.getline(buf, BUFSIZE);
  std::string strbuf(buf);

  while (strbuf.size() > 0) {
    size_t pos = strbuf.find_first_of(" ");
    if (pos == std::string::npos) {
      pos = strbuf.size(); // We have reached the end
    }

    std::string freq = strbuf.substr(0, pos);
    int freqKHz = atoi(freq.c_str());
    int freqMHz = freqKHz / 1000;

    // Hack to exclude turbo freqs
    if (freqMHz % 100 == 0) {
      freqs.push_back(freqMHz);
    }

    if (pos < strbuf.size() - 1) {
      strbuf = strbuf.substr(pos + 1, strbuf.size() - pos - 1);
    } else {
      strbuf = ""; // We have reached the end
    }
  }

  std::sort(freqs.begin(), freqs.end());

  return freqs;
}
