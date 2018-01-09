#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

#include "cmt.h"

void accessCacheSeqRaw() {
  const int64_t arrsize = 1LL << 26;

  // volatile int64_t* vals = new int64_t[arrsize];
  // for (int64_t i = 0; i < arrsize; ++i) vals[i] = rand();

  std::vector<int64_t> arr;
  arr.resize(arrsize);

  for (auto &v : arr)
    v = rand();
}

void accessCacheSeq() {
  const int64_t arrsize = 1LL << 26;
  std::vector<int64_t> vals;
  vals.resize(arrsize);

  for (auto &v : vals)
    v = rand();

  int res = 0;

  for (int64_t i = 0; i < 1; ++i) {
    for (int64_t j = 0; j < arrsize; ++j) {
      res = (res + vals[j]) / 2;
    }
  }

  (void) res;
}

void accessCache() {
  const int64_t arrsize = 1LL << 26;
  std::vector<int64_t> vals;
  std::vector<int64_t> idxs;
  vals.resize(arrsize);
  idxs.resize(arrsize);

  for (auto &v : vals)
    v = rand();
  for (auto &i : idxs)
    i = rand() % arrsize;

  int res = 0;

  for (int64_t i = 0; i < arrsize; ++i) {
    res = (res + vals[idxs[i]]) / 2;
  }

  (void) res;
}

int main(int argc, char *argv[]) {
  CMTController cmt_ctrl;
  uint64_t rmid = 0;

  cmt_ctrl.setGlobalRmid(rmid);
  struct timespec begin, end;
  int64_t mem_bytes_begin, mem_bytes_end;

  clock_gettime(CLOCK_REALTIME, &begin);
  mem_bytes_begin = cmt_ctrl.getLocalMemTraffic(rmid);

  accessCacheSeqRaw();
  // accessCacheSeq();

  clock_gettime(CLOCK_REALTIME, &end);
  mem_bytes_end = cmt_ctrl.getLocalMemTraffic(rmid);

  double mem_traffic_bytes = mem_bytes_end - mem_bytes_begin;
  double elapsed = (end.tv_sec - begin.tv_sec) * 1e3; // ms
  elapsed += (end.tv_nsec - begin.tv_nsec) * 1e-6;
  double mem_bw = mem_traffic_bytes / elapsed * (1e-3); // mbps

  std::cout << "Local Memory Traffic = " << mem_traffic_bytes << " bytes"
            << std::endl;
  std::cout << "Elapsed Time = " << elapsed << " msec" << std::endl;
  std::cout << "Bandwidth = " << mem_bw << " mbps" << std::endl;

  return 0;
}
