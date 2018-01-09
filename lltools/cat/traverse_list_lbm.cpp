#include <ctime>
#include <iostream>
#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <random>

#include "cmt.h"

struct ListElem {
  struct ListElem *next;
};

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

  (void) res; // So gcc doesn't optimize all of the preceding code away
}

void swap(ListElem &a, ListElem &b) {
  ListElem *tmp1 = a.next;
  ListElem *tmp2 = b.next;
  b.next = b.next->next;
  a.next = tmp2;
  a.next->next = tmp1;
}

ListElem *createCircularList(size_t elems) {
  ListElem *listArray = (ListElem *)calloc(elems, sizeof(ListElem));

  std::mt19937 gen(0xB1C5A11D1E);
  std::uniform_int_distribution<uint32_t> d(0, 1 << 30);

  // Initialize with contiguous, circular refs and random data
  for (uint32_t i = 0; i < elems; i++) {
    listArray[i].next = &listArray[(i + 1) % elems];
  }
  //listArray[elems-1].next = NULL;
  // Shuffle randomly (Fisher-Yates shuffle)
  for (uint32_t i = elems - 2; i > 0; i--) {
    uint32_t j = d(gen) % i; // j is in [0,...,i)
    swap(listArray[i], listArray[j]);
  }
  return listArray;
}

int main(int argc, char *argv[]) {
  CMTController cmt_ctrl;
  uint64_t rmid = 0;

  cmt_ctrl.setGlobalRmid(rmid);
  struct timespec begin, end;
  int64_t mem_bytes_begin, mem_bytes_end;

  if (argc < 2) {
    printf("Usage: %s <list size in KB>\n", argv[0]);
    exit(1);
  }

  // Create circular list of specified size
  size_t listKBs = atol(argv[1]);
  size_t listElems = listKBs * 1024ul / sizeof(ListElem);
  std::cout << "listElems = " << listElems
            << " (ListElem size = " << sizeof(ListElem) << ")" << std::endl;
  ListElem *head = createCircularList(listElems);

  // Traverse circular list
  ListElem *cur = head;

  // We want to do this, but the compiler's optimizer sees this code has no
  // effect and removes it...
  // while (true) {
  //     cur = cur->next;
  // }

  // Here's a version that confuses the optimizer enough and performs
  // multiple traversals per iteration

  clock_gettime(CLOCK_REALTIME, &begin);
  mem_bytes_begin = cmt_ctrl.getLocalMemTraffic(rmid);

  //    accessCacheSeq();
  for (int i = 0; i < 1000ul * 12 * 1024 / listKBs; i++) {
    cur = head;
    int j = 0;
    while (j < listElems)
      j++, cur = cur->next; // printf("%d\n", cur-head);
  }

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
  printf("Cur: %p\n", cur);

  return 0;
}
