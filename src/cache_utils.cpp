/** $lic$
* MIT License
*
* Copyright (c) 2017-2018 by Massachusetts Institute of Technology
* Copyright (c) 2017-2018 by Qatar Computing Research Institute, HBKU
*
* This file is part of KPart.
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

#include <sstream>
#include <string>
#include <string.h>
#include <iostream>
#include "cache_utils.h"
#ifdef USE_CMT
#include "cmt.h"
#endif

namespace cache_utils {

int share_all_cache_ways() { // Share all ways!
  if (enableLogging) {
    printf("[INFO]  Inside resetCacheWaysAllCores()\n");
  }
  int sts;

  for (int cosID = 0; cosID < NUM_CORES; cosID++) {
    int limit = CACHE_WAYS - 1;
    std::string waysString = "";
    for (int i = 0; i < limit; i++) {
      waysString = waysString + std::to_string(i);
      waysString += ",";
    }

    waysString = waysString + std::to_string(CACHE_WAYS - 1);
    waysString = " " + waysString;

    std::string rs =
        CAT_CBM_TOOL_DIR + std::to_string(cosID) + " -m" + waysString;
    sts = system(rs.c_str());
    if (enableLogging) {
      printf("[INFO] Changing COS %d to %d ways. Status= %d \n", cosID,
             CACHE_WAYS, sts);
    }
    rs = CAT_COS_TOOL_DIR + std::to_string(cosID) + " -s " +
         std::to_string(cosID);
    sts = system(rs.c_str());
    if (enableLogging) {
      printf("[INFO] Changing CORE %d map to COS %d. Status= %d \n", cosID,
             cosID, sts);
    }
  }
  return sts;
}

void get_maxmarginalutil_mrcs(arma::vec curve, int cur, int parts,
                              double result[]) {
  double maxMu = 1000;
  double maxMuPart = 0;

  for (int p = 1; p < (parts + 1); p++) {
    double mu = (double)(curve[(cur + p - 1)] - curve[(cur - 1)]) / (double) p;
    if (maxMu == 1000 || (mu < maxMu)) {
      maxMu = mu;
      maxMuPart = p;
    }
  }

  result[0] = maxMu;
  result[1] = maxMuPart;
}

void smoothenMRCs(arma::mat &mpkiVsWays) {
  if (enableLogging)
    printf("[INFO]  Inside smoothenMRCs()\n");
  double prevValue = 0.0;
  for (int j = 0; j < mpkiVsWays.n_cols; j++) {
    for (int i = 1; i < mpkiVsWays.n_rows; i++) {
      prevValue = mpkiVsWays(i - 1, j);
      mpkiVsWays(i, j) = std::min(prevValue, mpkiVsWays(i, j));
    }
  }
  if (enableLogging)
    mpkiVsWays.print();
}

void get_maxmarginalutil_ipcs(arma::vec curve, int cur, int parts,
                              double result[]) {
  double maxMu = 1000;
  double maxMuPart = 0;
  int baseIPCallocation = 3;

  for (int p = 1; p < (parts + 1); p++) {
    double mu = (double)(curve[(cur + p - 1)] - curve[(cur - 1)]) /
                (double)(p * curve[(baseIPCallocation - 1)]);

    if (maxMu == 1000 || (mu > maxMu)) {
      maxMu = mu;
      maxMuPart = p;
    }
  }

  result[0] = maxMu;
  result[1] = maxMuPart;
}

void smoothenIPCs(arma::mat &ipcVsWays) {
  if (enableLogging)
    printf("[INFO] Inside smoothenIPCs()\n");
  double prevValue = 0.0;
  for (int j = 0; j < ipcVsWays.n_cols; j++) {
    for (int i = 1; i < ipcVsWays.n_rows; i++) {
      prevValue = ipcVsWays(i - 1, j);
      ipcVsWays(i, j) = std::max(prevValue, ipcVsWays(i, j));
    }
  }
  if (enableLogging)
    ipcVsWays.print();
}

std::vector<std::vector<double> > get_wscurves_for_combinedmrcs(
    std::vector<std::vector<std::vector<std::pair<uint32_t, uint32_t> > > >
        cluster_bucks,
    arma::mat ipcVsWays) {
  int APP_RELATIVE_IPC_BUCK = 2;

  if (enableLogging) {
    printf("[INFO]  Inside get_wscurves_for_combinedmrcs():\n");
  }

  std::vector<std::vector<double> > wsCurveVecDbl;
  arma::vec ipcApp;
  std::vector<double> wsCurve(CACHE_WAYS);

  for (uint32_t cid = 0; cid < cluster_bucks.size(); cid++) {
    for (uint32_t p = 0; p < cluster_bucks[cid].size(); p++) {
      double ws = 0.0;
      for (uint32_t z = 0; z < cluster_bucks[cid][p].size(); z++) {
        int appIdx = cluster_bucks[cid][p][z].first;
        int appBucks = cluster_bucks[cid][p][z].second;
        ipcApp = ipcVsWays.col(appIdx);
        double appWs =
            (double) ipcApp[appBucks] / ipcApp[APP_RELATIVE_IPC_BUCK];
        ws += appWs;
      }
      wsCurve[p] = ws;
    }
    //std::cout << std::endl;
    wsCurveVecDbl.push_back(wsCurve);
  }

  return wsCurveVecDbl;
}

void apply_partition_plan(std::stack<int> partitions[]) {
  std::string waysString;
  int cosID, status;
  std::stack<int> appPartitions;

  for (int a = 0; a < NUM_CORES; ++a) {
    appPartitions = partitions[a];
    cosID = a;
    waysString = "";
    while (!appPartitions.empty()) {
      waysString = waysString + std::to_string(appPartitions.top());
      waysString += ",";
      appPartitions.pop();
    }

    waysString.erase(waysString.end() - 1);
    waysString = " " + waysString;

    std::string rs = CAT_CBM_TOOL_DIR + std::to_string(cosID);
    rs += " -m" + waysString;

    status = system(rs.c_str());
    if (enableLogging) {
      printf("[INFO] Changing cache alloc for cos %d to %s ways. Status= %d \n",
             cosID, waysString.c_str(), status);
    }

    if (status != 0)
      break;
  }

  //Now map each core to its COS
  for (int cosID = 0; cosID < NUM_CORES; cosID++) {
    std::string rs = CAT_COS_TOOL_DIR + std::to_string(cosID) + " -s " +
                     std::to_string(cosID);
    status = system(rs.c_str());
    if (enableLogging) {
      printf("[INFO] Changing CORE %d map to COS %d. Status= %d \n", cosID,
             cosID, status);
    }
  }
}

void print_allocations(uint32_t *allocs) {
  for (int i = 0; i < NUM_CORES; i++) {
    std::cout << allocs[i] << ", ";
  }
  std::cout << std::endl;
}

void do_ucp_mrcs(arma::mat mpkiVsWays) {
  if (enableLogging)
    printf("[INFO]  Inside do_ucp_mrcs()\n");
  smoothenMRCs(mpkiVsWays);

  int numApps = mpkiVsWays.n_cols;
  std::stack<int> buckets;
  for (int i = (CACHE_WAYS - 1); i >= 0; --i)
    buckets.push(i);
  std::stack<int> partitions[NUM_CORES];
  for (int a = 0; a < numApps; ++a) {
    partitions[a].push(buckets.top());
    buckets.pop();
  }

  while (!buckets.empty()) {
    double maxMu = 1000;
    int maxMuAppIdx;
    int maxMuBuckets = 0;

    for (int a = 0; a < numApps; a++) {
      //Extract app MRC:
      arma::vec mrcApp = mpkiVsWays.col(a);

      //get max marginal utility
      std::stack<int> appPartitions = partitions[a];
      int curBuckets = appPartitions.size();
      double rs[2];
      get_maxmarginalutil_mrcs(mrcApp, curBuckets, buckets.size(), rs);
      double mu = rs[0];
      double addBuckets = rs[1];

      if (maxMu == 1000 || (mu < maxMu)) {
        maxMu = mu;
        maxMuAppIdx = a;
        maxMuBuckets = addBuckets;
      }
    }

    for (int b = 0; b < maxMuBuckets; b++) {
      partitions[maxMuAppIdx].push(buckets.top());
      buckets.pop();
    }
    std::cout << '\n';
  }

  // --- Workaround bug with COS 10,11 in Intel's CAT --- //
  std::stack<int> appParts;
  int victimId_b10, victimId_b11 = -1;
  int victimPartSize_b10, victimPartSize_b11 = -1;
  int buckID = -1;

  for (int a = 0; a < numApps; ++a) {
    appParts = partitions[a];
    while (!appParts.empty()) {
      buckID = appParts.top();
      if (buckID == 10) {
        victimId_b10 = a;
        victimPartSize_b10 = partitions[a].size();
      }
      if (buckID == 11) {
        victimId_b11 = a;
        victimPartSize_b11 = partitions[a].size();
      }
      appParts.pop();
    }
  }
  if (victimId_b10 != victimId_b11) {
    // We got a problem, workaround CAT bug
    int swapMe = -1;
    bool notFound = true;

    // Swap with the other victim partition
    if (victimPartSize_b10 > 1) {
      appParts = partitions[victimId_b10]; //3,8,9,10
      while (!partitions[victimId_b10].empty())
        partitions[victimId_b10].pop();

      while (!appParts.empty()) {
        buckID = appParts.top();
        if (buckID != 10 && notFound) {
          swapMe = buckID;
          notFound = false;
          appParts.pop();
        } else {
          partitions[victimId_b10].push(buckID);
          appParts.pop();
        }
      }
      partitions[victimId_b10].push(11);
      appParts = partitions[victimId_b11];
      while (!partitions[victimId_b11].empty())
        partitions[victimId_b11].pop();

      while (!appParts.empty()) {
        buckID = appParts.top();
        if (buckID != 11) {
          partitions[victimId_b11].push(buckID);
        }
        appParts.pop();
      }
      partitions[victimId_b11].push(swapMe);
    } else if (victimPartSize_b11 > 1) {
      // Safe to assume here that partition with 10 is single
      appParts = partitions[victimId_b11]; //3,8,9,10
      while (!partitions[victimId_b11].empty())
        partitions[victimId_b11].pop();

      while (!appParts.empty()) {
        buckID = appParts.top();
        if (buckID != 11 && notFound) {
          swapMe = buckID;
          notFound = false;
          appParts.pop();
        } else {
          partitions[victimId_b11].push(buckID);
          appParts.pop();
        }
      }
      partitions[victimId_b11].push(10);
      appParts = partitions[victimId_b10];
      while (!partitions[victimId_b10].empty())
        partitions[victimId_b10].pop();

      while (!appParts.empty()) {
        buckID = appParts.top();
        if (buckID != 10) {
          partitions[victimId_b10].push(buckID);
        }
        appParts.pop();
      }
      partitions[victimId_b10].push(swapMe);
    } else { //10 in a single-buck partition, 11 in another single-buck
             //partition ...

      // Pick another victim partition with size > 2:
      int swap1, swap2 = -1;
      bool done = false;
      for (int a = 0; a < numApps; ++a) {
        if (a != victimId_b10 && a != victimId_b11 &&
            partitions[a].size() >= 2) {
          swap1 = partitions[a].top(); //get element1
          partitions[a].pop();
          swap2 = partitions[a].top(); //get element2
          partitions[a].pop();

          partitions[a].push(10);
          partitions[a].push(11);

          partitions[victimId_b10].pop();
          partitions[victimId_b10].push(swap1);

          partitions[victimId_b11].pop();
          partitions[victimId_b11].push(swap2);
          done = true;
        }

        if (done)
          break;
      }
    }
  }

  apply_partition_plan(partitions);

  if (enableLogging)
    printf("\n ------------- Cache assignments --------------  \n");

  for (int a = 0; a < numApps; ++a) {
    std::stack<int> appPartitions = partitions[a];
    while (!appPartitions.empty()) {
      std::cout << ' ' << appPartitions.top();
      appPartitions.pop();
    }
    std::cout << '\n';
  }
}

void do_ucp_ipcs(arma::mat ipcVsWays) {
  if (enableLogging)
    printf("[INFO]  Inside do_ucp_ipcs()\n");

  smoothenIPCs(ipcVsWays);
  int numApps = ipcVsWays.n_cols;
  std::stack<int> buckets;
  for (int i = (CACHE_WAYS - 1); i >= 0; --i)
    buckets.push(i);

  std::stack<int> partitions[NUM_CORES];
  std::stack<int> partitionsFixed[NUM_CORES];

  for (int a = 0; a < numApps; ++a) {
    partitions[a].push(buckets.top());
    buckets.pop();
  }

  while (!buckets.empty()) {
    double maxMu = 1000;
    int maxMuAppIdx;
    int maxMuBuckets = 0;

    for (int a = 0; a < numApps; a++) {
      arma::vec ipcApp = ipcVsWays.col(a);
      //get max marginal utility
      std::stack<int> appPartitions = partitions[a];
      int curBuckets = appPartitions.size();
      double rs[2];
      get_maxmarginalutil_ipcs(ipcApp, curBuckets, buckets.size(), rs);

      double mu = rs[0];
      double addBuckets = rs[1];

      if (maxMu == 1000 || (mu > maxMu)) {
        maxMu = mu;
        maxMuAppIdx = a;
        maxMuBuckets = addBuckets;
      }
    }

    for (int b = 0; b < maxMuBuckets; b++) {
      partitions[maxMuAppIdx].push(buckets.top());
      buckets.pop();
    }
    std::cout << '\n';
  }

  // --- Workaround bug with COS 10,11 in Intel's CAT --- //
  std::stack<int> appParts;
  int victimId_b10, victimId_b11 = -1;
  int victimPartSize_b10, victimPartSize_b11 = -1;
  int buckID = -1;

  for (int a = 0; a < numApps; ++a) {
    appParts = partitions[a];
    while (!appParts.empty()) {
      buckID = appParts.top();
      if (buckID == 10) {
        victimId_b10 = a;
        victimPartSize_b10 = partitions[a].size();
      }
      if (buckID == 11) {
        victimId_b11 = a;
        victimPartSize_b11 = partitions[a].size();
      }
      appParts.pop();
    }
  }

  if (victimId_b10 != victimId_b11) {
    // We got a problem, workaround CAT bug
    int swapMe = -1;
    bool notFound = true;

    // Swap with the other victim partition
    if (victimPartSize_b10 > 1) {
      appParts = partitions[victimId_b10]; //3,8,9,10
      while (!partitions[victimId_b10].empty())
        partitions[victimId_b10].pop();

      while (!appParts.empty()) {
        buckID = appParts.top();
        if (buckID != 10 && notFound) {
          swapMe = buckID;
          notFound = false;
          appParts.pop();
        } else {
          partitions[victimId_b10].push(buckID);
          appParts.pop();
        }
      }
      partitions[victimId_b10].push(11);
      appParts = partitions[victimId_b11];
      while (!partitions[victimId_b11].empty())
        partitions[victimId_b11].pop();

      while (!appParts.empty()) {
        buckID = appParts.top();
        if (buckID != 11) {
          partitions[victimId_b11].push(buckID);
        }
        appParts.pop();
      }
      partitions[victimId_b11].push(swapMe);
    } else if (victimPartSize_b11 > 1) {
      // Safe to assume here that partition with 10 is single
      appParts = partitions[victimId_b11]; //3,8,9,10
      while (!partitions[victimId_b11].empty())
        partitions[victimId_b11].pop();

      while (!appParts.empty()) {
        buckID = appParts.top();
        if (buckID != 11 && notFound) {
          swapMe = buckID;
          notFound = false;
          appParts.pop();
        } else {
          partitions[victimId_b11].push(buckID);
          appParts.pop();
        }
      }
      partitions[victimId_b11].push(10);
      appParts = partitions[victimId_b10];
      while (!partitions[victimId_b10].empty())
        partitions[victimId_b10].pop();

      while (!appParts.empty()) {
        buckID = appParts.top();
        if (buckID != 10) {
          partitions[victimId_b10].push(buckID);
        }
        appParts.pop();
      }
      partitions[victimId_b10].push(swapMe);
    } else { //10 in a single-buck partition, 11 in another single-buck
             //partition ...

      // Pick another victim partition with size > 2:
      int swap1, swap2 = -1;
      bool done = false;
      for (int a = 0; a < numApps; ++a) {
        if (a != victimId_b10 && a != victimId_b11 &&
            partitions[a].size() >= 2) {
          swap1 = partitions[a].top(); //get element1
          partitions[a].pop();
          swap2 = partitions[a].top(); //get element2
          partitions[a].pop();

          partitions[a].push(10);
          partitions[a].push(11);

          partitions[victimId_b10].pop();
          partitions[victimId_b10].push(swap1);

          partitions[victimId_b11].pop();
          partitions[victimId_b11].push(swap2);
          done = true;
        }

        if (done)
          break;
      }
    }
  }

  apply_partition_plan(partitions);

  printf("\n [INFO] ------------- Cache assignments --------------  \n");
  for (int a = 0; a < numApps; ++a) {
    std::stack<int> appPartitions = partitions[a];
    while (!appPartitions.empty()) {
      std::cout << ' ' << appPartitions.top();
      appPartitions.pop();
    }
    std::cout << '\n';
  }

}

std::string get_cacheways_for_core(int coreIdx) {
  std::string getCOSstring =
      "../lltools/build/cat_cbm -g -c " +
      std::to_string(coreIdx); //Assumption: coreIdx = COS idx
  int sts = system(getCOSstring.c_str());
  if (enableLogging) {
    printf("[INFO] Inside getCacheWays(). Get ways, status= %d \n", sts);
  }

  return getCOSstring;
}

// --- Workaround bug with COS 10,11 in Intel's CAT --- //
//void verify_intel_cos_issue(std::stack<int> [] & cluster_partitions, uint32_t
//K){
void verify_intel_cos_issue(std::stack<int> *cluster_partitions, uint32_t K) {
  std::stack<int> clustParts;
  int victimId_b10 = -1;
  int victimId_b11 = -1;
  int victimPartSize_b10 = -1;
  int victimPartSize_b11 = -1;
  int buckID = -1;
  int myK = (int) K;
  for (int a = 0; a < myK; ++a) {
    clustParts = cluster_partitions[a];
    while (!clustParts.empty()) {
      buckID = clustParts.top();
      if (buckID == 10) {
        victimId_b10 = a;
        victimPartSize_b10 = cluster_partitions[a].size();
      }
      if (buckID == 11) {
        victimId_b11 = a;
        victimPartSize_b11 = cluster_partitions[a].size();
      }
      clustParts.pop();
    }
  }

  if (victimId_b10 != victimId_b11) {
    // We got a problem, workaround CAT bug
    int swapMe = -1;
    bool notFound = true;

    // Swap with the other victim partition
    if (victimPartSize_b10 > 1) {
      clustParts = cluster_partitions[victimId_b10]; //3,8,9,10
      while (!cluster_partitions[victimId_b10].empty())
        cluster_partitions[victimId_b10].pop();

      while (!clustParts.empty()) {
        buckID = clustParts.top();
        if (buckID != 10 && notFound) {
          swapMe = buckID;
          notFound = false;
          clustParts.pop();
        } else {
          cluster_partitions[victimId_b10].push(buckID);
          clustParts.pop();
        }
      }
      cluster_partitions[victimId_b10].push(11);
      clustParts = cluster_partitions[victimId_b11];
      while (!cluster_partitions[victimId_b11].empty())
        cluster_partitions[victimId_b11].pop();

      while (!clustParts.empty()) {
        buckID = clustParts.top();
        if (buckID != 11) {
          cluster_partitions[victimId_b11].push(buckID);
        }
        clustParts.pop();
      }
      cluster_partitions[victimId_b11].push(swapMe);
    } else if (victimPartSize_b11 > 1) {
      // Safe to assume here that partition with 10 is single
      clustParts = cluster_partitions[victimId_b11]; //3,8,9,10
      while (!cluster_partitions[victimId_b11].empty())
        cluster_partitions[victimId_b11].pop();

      while (!clustParts.empty()) {
        buckID = clustParts.top();
        if (buckID != 11 && notFound) {
          swapMe = buckID;
          notFound = false;
          clustParts.pop();
        } else {
          cluster_partitions[victimId_b11].push(buckID);
          clustParts.pop();
        }
      }
      cluster_partitions[victimId_b11].push(10);
      clustParts = cluster_partitions[victimId_b10];
      while (!cluster_partitions[victimId_b10].empty())
        cluster_partitions[victimId_b10].pop();

      while (!clustParts.empty()) {
        buckID = clustParts.top();
        if (buckID != 10) {
          cluster_partitions[victimId_b10].push(buckID);
        }
        clustParts.pop();
      }
      cluster_partitions[victimId_b10].push(swapMe);
    } else { //10 in a single-buck partition, 11 in another single-buck
             //partition ...
             // Pick another victim partition with size > 2:
      int swap1, swap2 = -1;
      bool done = false;
      for (int a = 0; a < myK; ++a) {
        if (a != victimId_b10 && a != victimId_b11 &&
            cluster_partitions[a].size() >= 2) {
          swap1 = cluster_partitions[a].top(); //get element1
          cluster_partitions[a].pop();
          swap2 = cluster_partitions[a].top(); //get element2
          cluster_partitions[a].pop();

          cluster_partitions[a].push(10);
          cluster_partitions[a].push(11);

          cluster_partitions[victimId_b10].pop();
          cluster_partitions[victimId_b10].push(swap1);

          cluster_partitions[victimId_b11].pop();
          cluster_partitions[victimId_b11].push(swap2);
          done = true;
        }

        if (done)
          break;
      }
    }
  }
  //return cluster_partitions;
}

} // namespace cache_utils
