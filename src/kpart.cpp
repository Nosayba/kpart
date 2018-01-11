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
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/wait.h>
#include <locale.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sched.h>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>
#include <kpart.h>
#include <cluster/hill_climb.h>
#include <cluster/lookahead.h>
#include <cluster/lookahead.cpp>
#include <cluster/hcluster.cpp>
#include <cluster/miss_curve.cpp>
#include <cluster/whirlpool.cpp>
#include <cluster/whirlpool.h>
#include <cluster/hcluster.h>
#include <armadillo>
using namespace arma;

extern "C" {
#include "perf_util.h"
}

#ifdef USE_CMT
#include "cmt.h"
#endif

//Monitoring and profiling variables
bool enableLogging(true); //Turn on for detailed logging of profiling
bool estimateMRCenabled(true);
bool monitorStartFlag(false);
bool sample12to1(true);
bool geometricSamplePlan(false); //test: geometric sampling 1,2,3,5,8,10,11
bool doMorePartitioning(true);

int monitorLen = 1; //estimate MRC IPC point every monitorLen phases
int sampleSlicesIdx = -1;
uint32_t K =
    2; // Default number of clusters is 2, but changes in Kauto calculations
int numSamplesBeforePartitioning = 1;
bool firstInvokation = true;
int numSamples = 0;
int numProcesses = 0; //updated dynamically when processes are launched
int procIdxProfiled_global =
    0;                //starts with process 0, up to (computed) numProcesses

int numWaysToSample = 7;
arma::cube allAppsCacheAssignments(2, CACHE_WAYS, numWaysToSample, fill::zeros);
arma::mat currentlySampling = zeros<arma::mat>(NUM_CORES, 2);
arma::mat sampledMRCs = zeros<arma::mat>(CACHE_WAYS, NUM_CORES);
arma::mat sampledIPCs = zeros<arma::mat>(CACHE_WAYS, NUM_CORES);
arma::vec loggingMRCFlags = zeros<arma::vec>(NUM_CORES);

// Will be set according to user input:
int invokeMonitorLen = -1; //Skip this much instructions before invoking DynaWay
int warmUpInterval = -1;
int profileInterval = -1;

// NOTE: The 'master' mode, enabled via MASTER_PROC, treats the first process
// (pidx 0) as the master process and everybody else as the slave. Sage runs the
// app mix until pidx 0 finishes (either via reaches maxPhases or before that),
// and then kills all the processes. Slave processes keep running, and looging
// data, while the master is running.

const int SIGSAGE = SIGRTMIN + 1;

struct ProcessInfo {
  int pid;
  int pidx; // Process indices in order specified on cmd line
  std::string input;
  std::vector<int> cores;
  perf_event_desc_t *fds;
  int numPhases;
  int maxPhases;
  std::vector<char *> args;
  std::vector<uint64_t> values;
  FILE *logFd;

  FILE *mrcfd;
  FILE *ipcfd;

  uint64_t lastInstrCtr;
  uint64_t lastCyclesCtr;
  uint64_t lastMemTrafficCtr;

  arma::vec xPoints = arma::linspace<arma::vec>(0, 0, numWaysToSample);
  arma::vec yPoints_ipc = arma::linspace<arma::vec>(0, 0, numWaysToSample);
  arma::vec yPoints_mpki = arma::linspace<arma::vec>(0, 0, numWaysToSample);

  arma::vec mrcEstAvg = zeros<arma::vec>(CACHE_WAYS);
  arma::mat mrcEstimates = zeros<arma::mat>(CACHE_WAYS, 1000);

  arma::vec ipcCurveAvg = zeros<arma::vec>(CACHE_WAYS);
  arma::mat ipcCurveEstimates = zeros<arma::mat>(CACHE_WAYS, 1000);

  int mrcEstIndex;
  int pSampleSlicesIdx;

#ifdef USE_CMT
  int rmid;

  int64_t memTrafficLast;
  int64_t memTrafficTotal;

  int64_t avgCacheOccupancy;
#endif

  ProcessInfo()
      : pid(-1), pidx(-1), fds(nullptr), numPhases(0), maxPhases(-1),
        logFd(nullptr), mrcfd(nullptr), ipcfd(nullptr), lastInstrCtr(0),
        lastCyclesCtr(0), lastMemTrafficCtr(0), mrcEstIndex(0),
        pSampleSlicesIdx(0)
#ifdef USE_CMT
        ,
        rmid(-1), memTrafficLast(0), memTrafficTotal(0), avgCacheOccupancy(0)
#endif
        {
  }

  void flush() {
    fflush(logFd);
    fflush(mrcfd); //Nosayba: added
    fflush(ipcfd); //Nosayba: added
  }

  ~ProcessInfo() { flush(); }

};

#ifdef USE_CMT

CMTController cmtCtrl;
const int64_t MEM_TRAFFIC_MAX = 1L << 39;
std::string lmbName = "LOCAL_MEM_TRAFFIC";
std::string l3OccupName = "L3_OCCUPANCY";

void updateMemTraffic(ProcessInfo &pinfo) {
  int64_t memTraffic = cmtCtrl.getLocalMemTraffic(pinfo.rmid);

  if (memTraffic < pinfo.memTrafficLast) {
    pinfo.memTrafficTotal += (MEM_TRAFFIC_MAX - pinfo.memTrafficLast);
    pinfo.memTrafficTotal += memTraffic;
  } else {
    pinfo.memTrafficTotal += (memTraffic - pinfo.memTrafficLast);
  }

  pinfo.memTrafficLast = memTraffic;
}

void updateCacheOccupancy(ProcessInfo &pinfo) {
  pinfo.avgCacheOccupancy = cmtCtrl.getLlcOccupancy(pinfo.rmid);
}

void initCmt(ProcessInfo &pinfo) {
  for (int c : pinfo.cores) {
    cmtCtrl.setRmid(c, pinfo.rmid);
  }

  pinfo.memTrafficLast = cmtCtrl.getLocalMemTraffic(pinfo.rmid);
  pinfo.memTrafficTotal = 0;
  pinfo.avgCacheOccupancy = 0;
}

#endif

std::vector<ProcessInfo> processInfo;

int activeProcs = 0;
int numEvents = 0;
char *events = nullptr;
int64_t phaseLen = -1;
bool inRoi = false;

perf_event_desc_t *globFds;

bool pinProcesses = true;

std::string logFile;
bool prettyPrint;

std::unordered_map<int, ProcessInfo *> pidMap;

void global_setup_counters(const char *events);
void setup_counters(ProcessInfo &pinfo); //see below
void read_counters(ProcessInfo &pinfo);

void read_counters(ProcessInfo &pinfo) {
  if (pinfo.values.size() < numEvents)
    pinfo.values.resize(numEvents);

  for (uint32_t i = 0; i < numEvents; i++) {
    int ret = read(pinfo.fds[i].fd, &pinfo.values[i], sizeof(pinfo.values[i]));
    if (ret != sizeof(pinfo.values[i])) {
      if (ret == -1)
        errx(1, "cannot read values event %s", pinfo.fds[i].name);
      errx(1, "incorrect read of values event %s, %d", pinfo.fds[i].name, ret);
    }
  }

#ifdef USE_CMT
  updateMemTraffic(pinfo);
  updateCacheOccupancy(pinfo);
#endif
}

void dump_counters(ProcessInfo &pinfo) {
  int i;
  read_counters(pinfo);

  if (prettyPrint) {
    for (i = 0; i < numEvents; i++) {
      fprintf(pinfo.logFd, "%40s  %3d:%ld\n", pinfo.fds[i].name, pinfo.pid,
              pinfo.values[i]);
    }
#ifdef USE_CMT
    fprintf(pinfo.logFd, "%40s %3d:%ld\n", lmbName.c_str(), pinfo.pid,
            pinfo.memTrafficTotal);
    fprintf(pinfo.logFd, "%40s %d:%ld\n", l3OccupName.c_str(), pinfo.pid,
            pinfo.avgCacheOccupancy);
#endif
  } else {
    for (i = 0; i < numEvents - 1; i++)
      fprintf(pinfo.logFd, "%ld ", pinfo.values[i]);

#ifdef USE_CMT
    fprintf(pinfo.logFd, "%ld ", pinfo.values[numEvents - 1]);
    fprintf(pinfo.logFd, "%ld ", pinfo.memTrafficTotal);
    fprintf(pinfo.logFd, "%ld\n", pinfo.avgCacheOccupancy);
#else
    fprintf(pinfo.logFd, "%ld\n", pinfo.values[numEvents - 1]);
#endif
  }
}

// ----------- Helper Functions --------------------//
int resetCacheWaysAllCores() { // Share all ways!
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
}

std::vector<std::vector<double> > getWsCurvesForCombinedMRCs(
    std::vector<std::vector<std::vector<std::pair<uint32_t, uint32_t> > > >
        cluster_bucks,
    arma::mat ipcVsWays) {
  int APP_RELATIVE_IPC_BUCK = 2;

  if (enableLogging) {
    printf("[INFO]  Inside getWsCurvesForCombinedMRCs():\n");
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

void getMaxMarginalUtilforMRC(arma::vec curve, int cur, int parts,
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

void getMaxMarginalUtilforIPC(arma::vec curve, int cur, int parts,
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

void generateProfilingPlan(int cacheCapacity) {
  if (enableLogging) {
    printf("[INFO]  Inside generateProfilingPlan(%d) \n", cacheCapacity);
  }

  arma::vec plan;
  switch (cacheCapacity) {
  // Modify "plan" if you want DynaWay to sample cache sizes differently
  // Add cases if more/different cache capacities are passible
  case 12:
    plan = { 11, 11, 8, 6, 4, 2, 1 };
    break;
  case 11:
    plan = { 10, 10, 8, 6, 4, 2, 1 };
    break;
  case 10:
    plan = { 9, 9, 8, 6, 4, 2, 1 };
    break;
  case 9:
    plan = { 8, 8, 6, 4, 3, 2, 1 };
    break;
  case 8:
    plan = { 7, 7, 6, 4, 3, 2, 1 };
    break;
  case 7:
    plan = { 6, 6, 5, 4, 3, 2, 1 };
    break;
  case 6:
    plan = { 5, 5, 4, 3, 2, 1 };
    break;
  case 5:
    plan = { 4, 4, 3, 2, 1 };
    break;
  case 4:
    plan = { 3, 3, 2, 1 };
    break;
  case 3:
    plan = { 2, 2, 1 };
    break;
  default:
    errx(1, "Invalid cache capacity passed to generateProfilingPlan().");
  }

  numWaysToSample = plan.size();

  if (enableLogging) {
    plan.print();
    printf("[INFO] Num curve points to sample = %d\n", numWaysToSample);
  }

  // Resize data structures based on the new cache capacity available to batch
  // apps
  allAppsCacheAssignments.set_size(2, cacheCapacity, numWaysToSample);
  sampledMRCs.set_size(cacheCapacity, NUM_CORES);
  sampledIPCs.set_size(cacheCapacity, NUM_CORES);

  for (ProcessInfo &pinfoIter : processInfo) {
    // Resizing relevant data structures
    pinfoIter.mrcEstAvg.set_size(cacheCapacity);
    pinfoIter.mrcEstimates.set_size(cacheCapacity, 1000);
    pinfoIter.ipcCurveAvg.set_size(cacheCapacity);
    pinfoIter.ipcCurveEstimates.set_size(cacheCapacity, 1000);
    pinfoIter.mrcEstIndex = 0;
    pinfoIter.xPoints.set_size(numWaysToSample);
    pinfoIter.yPoints_ipc.set_size(numWaysToSample);
    pinfoIter.yPoints_mpki.set_size(numWaysToSample);
  }

  int row = -1;
  int idx = -1;
  arma::mat A(2, cacheCapacity);
  int sliceIdx = 0;

  for (int s = 0; s < numWaysToSample; s++) {
    // E.g.: if cache capacity = 6, plan = {5, 5, 4, 3, 2, 1};
    int p = plan[s];           // e.g. 5 (ways being profiled for target app)
    int r = cacheCapacity - p; // e.g. 1 (remaining ways for rest of apps)

    row = 0;
    idx = 0;
    for (int i = 0; i < p; i++) {
      A(row, idx) = 1;
      idx++;
    }
    for (int i = 0; i < r; i++) {
      A(row, idx) = 0;
      idx++;
    }
    //e.g. init: "1, 1, 1, 1, 1, 0"

    row = 1;
    idx = 0;
    for (int i = 0; i < p; i++) {
      A(row, idx) = 0;
      idx++;
    }
    for (int i = 0; i < r; i++) {
      A(row, idx) = 1;
      idx++;
    }
    //e.g. init: "1, 0, 0, 0, 0, 0"

    allAppsCacheAssignments.slice(sliceIdx) = A;
    sliceIdx++;
  }

  // workaround CAT bug with buckets 10,11
  A = { { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }; // 0:11, 1:1
  allAppsCacheAssignments.slice(0) = A;
  allAppsCacheAssignments.slice(1) = A;

}

void applyPartitioningPlan(std::stack<int> partitions[]) {
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

void printAllocs(uint32_t *allocs) {
  for (int i = 0; i < NUM_CORES; i++) {
    std::cout << allocs[i] << ", ";
  }
  std::cout << std::endl;
}
struct timeval startAll, endAll;
struct timeval startT, endT;
void startTime() { gettimeofday(&startT, 0); }
void stopTime(const char *name) {
  gettimeofday(&endT, 0);
  double time = (endT.tv_sec - startT.tv_sec) * 1e3 +
                (endT.tv_usec - startT.tv_usec) * 1e-3;
  printf("[TIMECALC] %s = %.3f ms\n", name, time);
}

void doClusteringForMRCs(arma::mat mpkiVsWays, arma::mat ipcVsWays) {
  if (enableLogging)
    printf("\n [INFO]  Inside doClusteringForMRCs()\n");
  smoothenMRCs(mpkiVsWays);
  smoothenIPCs(ipcVsWays);
  int numApps = mpkiVsWays.n_cols;

  // Convert sampled MRCs to format compatible with hclustering library:
  uint32_t numTimeIntervals = 1;
  std::vector<std::vector<RawMissCurve> > timeCurves;

  for (uint32_t i = 0; i < numApps; i++) {
    std::vector<uint32_t> data(CACHE_WAYS);
    std::copy(&mpkiVsWays.col(i)[0], &mpkiVsWays.col(i)[CACHE_WAYS],
              data.begin());
    std::vector<RawMissCurve> appCurves;
    for (uint32_t j = 0; j < numTimeIntervals; j++) {
      appCurves.push_back(RawMissCurve(std::move(data), nullptr));
    }
    timeCurves.push_back(appCurves);
  }

  if (enableLogging) {
    printf("[INFO]  printing appCurves:\n");
    for (uint32_t i = 0; i < timeCurves.size(); i++) {
      std::cout << "App " << i << " ";
      for (uint32_t j = 0; j < timeCurves[i].size(); j++) {
        std::cout << timeCurves[i][j];
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }

  // ************* AUTO-K CALC ************* //
  if (enableLogging)
    printf("\n[INFO] Auto-K Clustering ... \n");
  hcluster::HCluster clustauto;
  std::vector<hcluster::HCluster::results_pack> rpauto =
      clustauto.clusterAuto(timeCurves);

  // For each "K", get the expected WS
  // Then, rank K's by the corresponding WS and pick the number of clusters
  // that yields the maximum weighted spedup for this cluster
  double bestWs = -1.0;
  uint32_t bestK = 0;
  int bestKidx = -1.0;

  for (uint32_t k = 0; k < (NUM_CORES - 2); k++) {
    int num_clusters = NUM_CORES - (k + 1);

    if (enableLogging)
      printf("\n \t[========================  K = %d   "
             "========================]\n",
             num_clusters);

    hcluster::HCluster::results_pack rp = rpauto[k];
    auto item_to_clusts = rp.item_to_clusts;
    auto cluster_curves = rp.cluster_curves;
    auto cluster_bucks = rp.cluster_buckets;

    // Get partitions for per-cluster curves using WS curves
    std::vector<const MissCurve *> curveVec;
    for (uint32_t c = 0; c < num_clusters; c++) {
      //std::cout << "cluster ID = "<< c << " temp: "; //std::endl;
      std::vector<uint32_t> data(CACHE_WAYS);
      data = cluster_curves[c][0].yvals;
      curveVec.push_back(new RawMissCurve(std::move(data), nullptr));
    }
    std::vector<uint32_t> minAllocs;
    minAllocs.push_back(1);

    uint32_t allocations[num_clusters];
    std::vector<std::vector<double> > wsCurveVecDbl =
        getWsCurvesForCombinedMRCs(cluster_bucks, ipcVsWays);
    std::vector<const MissCurve *> wsCurveVec;
    for (uint32_t i = 0; i < wsCurveVecDbl.size(); i++) {
      std::vector<uint32_t> data(CACHE_WAYS);
      std::copy(&wsCurveVecDbl[i][0], &wsCurveVecDbl[i][CACHE_WAYS],
                data.begin());
      wsCurveVec.push_back(new RawMissCurve(std::move(data), nullptr));
    }

    hillClimbingPartitionWsCurves(CACHE_WAYS, minAllocs, &allocations[0],
                                  wsCurveVec, wsCurveVecDbl);

    //Given this partitioning plan, what's the corresponding total WS?
    double wsK = 0.0;
    for (uint32_t w = 0; w < num_clusters; w++) {
      int allocIdx = allocations[w] - 1;
      wsK += wsCurveVecDbl[w][allocIdx];
    }

    if (wsK > bestWs) {
      bestWs = wsK;
      bestK = num_clusters;
      bestKidx = k;
    }
    if (enableLogging)
      printf("\t\t=> For num_clusters = %d, predicted WS = %.2f\n",
             num_clusters, wsK);

  } //end of processing all K results returned by clusterAuto()
    // ************* End of AUTO-K calculations ************* //

  // Select this predicted K
  K = bestK;

  if (enableLogging)
    printf("\n[INFO] Cluster applications into K-Auto = %d groups ... \n", K);

  hcluster::HCluster::results_pack rp = rpauto[bestKidx];

  auto item_to_clusts = rp.item_to_clusts;
  auto cluster_curves = rp.cluster_curves;
  auto cluster_bucks = rp.cluster_buckets;

  if (enableLogging)
    printf("[INFO] Get partitions for per-cluster curves.. \n");

  std::vector<uint32_t> minAllocs;
  minAllocs.push_back(1);

  // Partitioning based on WS curves ..
  uint32_t allocations[K];
  std::vector<std::vector<double> > wsCurveVecDbl =
      getWsCurvesForCombinedMRCs(cluster_bucks, ipcVsWays);
  std::vector<const MissCurve *> wsCurveVec;
  for (uint32_t i = 0; i < wsCurveVecDbl.size(); i++) {
    std::vector<uint32_t> data(CACHE_WAYS);
    std::copy(&wsCurveVecDbl[i][0], &wsCurveVecDbl[i][CACHE_WAYS],
              data.begin());
    wsCurveVec.push_back(new RawMissCurve(std::move(data), nullptr));
  }
  hillClimbingPartitionWsCurves(CACHE_WAYS, minAllocs, &allocations[0],
                                wsCurveVec, wsCurveVecDbl);
  if (enableLogging) {
    printf("[INFO] Hill climbing on WS curves: ");
    printAllocs(allocations);
  }

  // Apply per-cluster partitioning; e.g.: for K=3: 9, 2, 1
  if (enableLogging)
    printf("[INFO] Apply per-cluster partitioning ... \n");

  std::stack<int> buckets;
  for (int i = (CACHE_WAYS - 1); i >= 0; --i)
    buckets.push(i);
  std::stack<int> cluster_partitions[K];
  for (uint32_t c = 0; c < K; c++) {
    for (uint32_t p = 0; p < allocations[c]; p++) {
      cluster_partitions[c].push(buckets.top());
      buckets.pop();
    }
  }

  // ------------------------------------------------------------------------ //
  // --- Workaround bug with COS 10,11 in Intel's CAT --- //
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
  // ------------------------------------------------------------------------ //

  std::stack<int> app_partitions[numApps];

  printf("\n ------------- KPart-DYN Cache assignments to apps --------------  "
         "\n");
  for (int a = 0; a < numApps; ++a) {
    int cid = item_to_clusts[a];
    app_partitions[a] = cluster_partitions[cid];
    std::cout << "App: " << a << " Clust: " << cid << " Parts: ";
    std::string waysString = "";
    while (!app_partitions[a].empty()) {
      std::cout << ' ' << app_partitions[a].top();
      waysString = waysString + std::to_string(app_partitions[a].top());
      waysString += ",";
      app_partitions[a].pop();
    }
    std::cout << std::endl;
  }

  for (int a = 0; a < numApps; ++a) {
    int cid = item_to_clusts[a];
    app_partitions[a] = cluster_partitions[cid];
  }

  // Now apply this partitioning plan:
  applyPartitioningPlan(app_partitions);

}

// ---------------------------------------------------------- //
void doUcpForMRCs(arma::mat mpkiVsWays) {
  if (enableLogging)
    printf("[INFO]  Inside doUcpForMRCs()\n");
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
      getMaxMarginalUtilforMRC(mrcApp, curBuckets, buckets.size(), rs);
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

  applyPartitioningPlan(partitions);

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
// ---------------------------------------------------------- //

// ---------------------------------------------------------- //
void doUcpForIPCs(arma::mat ipcVsWays) {
  if (enableLogging)
    printf("[INFO]  Inside doUcpForIPCs()\n");

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
      getMaxMarginalUtilforIPC(ipcApp, curBuckets, buckets.size(), rs);

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

  applyPartitioningPlan(partitions);

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

// ---------------------------------------------------------- //
int setCacheWaysAllCores(arma::mat C, int procIdxProfiled) {
  // E.g.  A = {  {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  //           {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1} }; // 0:1, 1:11
  std::string waysString;
  int cosID, numWaysBeingSampled, status;
  arma::mat cosMap = zeros<arma::mat>(C.n_rows, 2);

  // cosID = 0 has the sampled way string,
  // cosID = 1 should have the other way string with all remaining processes
  // sharing these ways ..
  for (cosID = 0; cosID < C.n_rows; cosID++) {
    waysString = "";
    numWaysBeingSampled = 0;
    for (int j = 0; j < C.n_cols; j++) {
      if (C(cosID, j) > 0) {
        numWaysBeingSampled++;
        waysString = waysString + std::to_string(j);
        waysString += ",";
      }
    }
    waysString.erase(waysString.end() - 1);
    waysString = " " + waysString;

    std::string rs = CAT_CBM_TOOL_DIR + std::to_string(cosID);
    rs += " -m" + waysString;

    status = system(rs.c_str());
    //if(enableLogging){ printf("[INFO] Changing cache alloc for cos %d to %s
    //ways. Status= %d \n", cosID, waysString.c_str(), status); }

    if (status != 0)
      break;

    //Indicate that this process is now sampling "x" number of cache ways
    //currentlySampling[cosID] = numWaysBeingSampled;
    cosMap(cosID, 0) = numWaysBeingSampled;
    cosMap(cosID, 1) = 1;
    //if(enableLogging){ printf("[INFO] Indicating that COS %d is now mapped to
    //%d ways.\n", cosID, numWaysBeingSampled); }
  }

  //Now map: (1) the profiled process (id: procIdxProfiled) to COS0,
  cosID = 0; //Assumption: profiled process will be mapped to COS0
  std::string rs = CAT_COS_TOOL_DIR + std::to_string(procIdxProfiled) + " -s " +
                   std::to_string(cosID);

  status = system(rs.c_str());
  if (enableLogging) {
    printf("[INFO] Changing CORE %d map to COS %d. Status= %d \n",
           procIdxProfiled, cosID, status);
  }

  currentlySampling(procIdxProfiled, 0) =
      cosMap(cosID, 0); //numWaysBeingSampled;
  currentlySampling(procIdxProfiled, 1) = 1;

  // (2) everyone else to COS1 to share the remaining ways
  cosID = 1; //Assumption: rest of processes will be sharing cache ways in COS 1

  for (int procID = 0; procID < NUM_CORES; procID++) {
    if (procID == procIdxProfiled)
      continue;

    rs = CAT_COS_TOOL_DIR + std::to_string(procID) + " -s " +
         std::to_string(cosID);
    status = system(rs.c_str());
    //if(enableLogging){ printf("[INFO] Changing CORE %d map to COS %d. Status=
    //%d \n", procID, cosID, status); }

    //Indicate that this process is now sampling "x" number of cache ways
    currentlySampling(procID, 0) = cosMap(cosID, 0); //numWaysBeingSampled;
    currentlySampling(procID, 1) = 1;
    //if(enableLogging){ printf("[INFO] Indicating that pid %d is now sampling
    //%d ways.\n", procID, numWaysBeingSampled); }
  }

  if (status != 0) {
    printf("[ERROR] Failed to change cache allocation for COS ID %d \n", cosID);
  }
  return status;
}

std::string getCacheWays(int coreIdx) {
  std::string getCOSstring =
      "../lltools/build/cat_cbm -g -c " +
      std::to_string(coreIdx); //Assumption: coreIdx = COS idx
  int sts = system(getCOSstring.c_str());
  if (enableLogging) {
    printf("[INFO] Inside getCacheWays(). Get ways, status= %d \n", sts);
  }

  return getCOSstring;
}
// ---------------------------------------------------------- //

// ---------------------------------------------------------- //
void dump_mrcEstimates(ProcessInfo &pinfo) {
  rewind(pinfo.mrcfd);

  // Smoothen before dumping:
  double prevValue = 0.0;
  for (int j = 0; j < pinfo.mrcEstIndex; j++) {
    for (int i = 1; i < pinfo.mrcEstimates.n_rows; i++) {
      prevValue = pinfo.mrcEstimates(i - 1, j);
      pinfo.mrcEstimates(i, j) = std::min(prevValue, pinfo.mrcEstimates(i, j));
    }
  }

  // dump to log file of online samples for this process
  for (int i = 0; i < pinfo.mrcEstimates.n_rows; i++) {
    for (int j = 0; j < pinfo.mrcEstIndex; j++) {
      fprintf(pinfo.mrcfd, "%f ", pinfo.mrcEstimates(i, j));
    }
    fprintf(pinfo.mrcfd, "\n");
  }
}

void dump_ipcEstimates(ProcessInfo &pinfo) {
  rewind(pinfo.ipcfd);

  // Smoothen before dumping:
  double prevValue = 0.0;
  for (int j = 0; j < pinfo.mrcEstIndex; j++) {
    for (int i = 1; i < pinfo.ipcCurveEstimates.n_rows; i++) {
      prevValue = pinfo.ipcCurveEstimates(i - 1, j);
      pinfo.ipcCurveEstimates(i, j) =
          std::max(prevValue, pinfo.ipcCurveEstimates(i, j));
    }
  }

  // dump to log file of online samples for this process
  for (int i = 0; i < pinfo.ipcCurveEstimates.n_rows; i++) {
    for (int j = 0; j < pinfo.mrcEstIndex; j++) {
      fprintf(pinfo.ipcfd, "%f ", pinfo.ipcCurveEstimates(i, j));
    }
    fprintf(pinfo.ipcfd, "\n");
  }
}
// ---------------------------------------------------------- //

// ---------------------------------------------------------- //
void sigsage_handler(int n, siginfo_t *info, void *vsc) {
  struct sigcontext *sc = reinterpret_cast<struct sigcontext *>(vsc);
  struct perf_event_mmap_page *hdr;
  struct perf_event_header ehdr;
  int id, ret;

  int pid = info->si_uid;
  int fd = info->si_fd;
  ProcessInfo &pinfo = *pidMap[fd];

  ++pinfo.numPhases;
  //printf("[TEST] PROC %d, PHASE %d", pinfo.pidx, pinfo.numPhases);
  //assert(pinfo.numPhases <= pinfo.maxPhases);

  // --------------------------------------------------- //
  if (!monitorStartFlag && pinfo.numPhases > 1) {
    pinfo.lastInstrCtr = pinfo.values[0];
    pinfo.lastCyclesCtr = pinfo.values[2];
#ifdef USE_CMT
    pinfo.lastMemTrafficCtr = pinfo.memTrafficTotal;
#endif
  }

  if (pinfo.numPhases % invokeMonitorLen == 0 && estimateMRCenabled) {
    pinfo.lastInstrCtr = pinfo.values[0];
    pinfo.lastCyclesCtr = pinfo.values[2];
#ifdef USE_CMT
    pinfo.lastMemTrafficCtr = pinfo.memTrafficTotal;
#endif

    if (pinfo.pidx == 0 && (pinfo.numPhases < pinfo.maxPhases)) { //Master
      if (enableLogging) {
        printf("\n[INFO] Master process invokes beginning of profiling for "
               "PROC %d, PHASE %d\n",
               procIdxProfiled_global, pinfo.numPhases);
      }

      if (firstInvokation) {
        invokeMonitorLen = profileInterval;
        firstInvokation = false;
      }

      startTime(); //calculate elapsed time for profiling episode

      monitorStartFlag = true;
      sampleSlicesIdx = 0;
      arma::mat C = allAppsCacheAssignments.slice(sampleSlicesIdx);
      //Slice has all cache assignments in a form of a matrix
      //Each row in the matrix corresponds to a given COS assignment

      setCacheWaysAllCores(C, procIdxProfiled_global);
      sampleSlicesIdx++;
    }
  } else if (monitorStartFlag && (pinfo.numPhases % monitorLen == 0)) {
    pinfo.xPoints[(sampleSlicesIdx - 1)] = currentlySampling(pinfo.pidx, 0);

    //BUG: APM8 w/onlineProf: sometimes counters don't get updated even though
    //process moved to next phase!
    //Workaround: Enable hyperthreading and pin KPart to a thread not being used
    //by a process being profiled
    if ((double)(pinfo.values[0] - pinfo.lastInstrCtr) == 0) {
      printf(" ### BUG ALERT WITH H/W COUNTERS ### pinfo.pidx = %d, "
             "pinfo.pnumPhases = %d, pinfo.values[0] (instr)=%f, "
             "pinfo.values[2] (cycles)=%f \n",
             pinfo.pidx, pinfo.numPhases, (double) pinfo.values[0],
             (double) pinfo.values[2]);
      currentlySampling(pinfo.pidx, 1) = 5; //Mark as incomplete with error ..
    } else { //Collect counters and mark as collected
      pinfo.yPoints_ipc[(sampleSlicesIdx - 1)] =
          (double)(pinfo.values[0] - pinfo.lastInstrCtr) /
          (double)(pinfo.values[2] - pinfo.lastCyclesCtr);
#ifdef USE_CMT
      double misses = (double)(pinfo.memTrafficTotal -
                               pinfo.lastMemTrafficCtr) / CACHE_LINE_SIZE;
      ;
      pinfo.yPoints_mpki[(sampleSlicesIdx - 1)] =
          misses * 1000 / (pinfo.values[0] - pinfo.lastInstrCtr);
#endif
      currentlySampling(pinfo.pidx, 1) = 0; //Collected, mark as completed!
    }

    pinfo.lastInstrCtr = pinfo.values[0];
    pinfo.lastCyclesCtr = pinfo.values[2];
#ifdef USE_CMT
    pinfo.lastMemTrafficCtr = pinfo.memTrafficTotal;
#endif

    if (enableLogging) {
      printf("[INFO] pinfo.pidx = %d, pinfo.pnumPhases = %d, "
             "sampledWays=%f,sampledIPC=%f, sampledMPKI=%f \n",
             pinfo.pidx, pinfo.numPhases, pinfo.xPoints[(sampleSlicesIdx - 1)],
             pinfo.yPoints_ipc[(sampleSlicesIdx - 1)],
             pinfo.yPoints_mpki[(sampleSlicesIdx - 1)]);
    }

    if (sampleSlicesIdx == numWaysToSample &&
        currentlySampling(pinfo.pidx, 1) != 5) {

      // Use collected MRC samples to estimate MRC only for one profiled process
      if (pinfo.pidx == procIdxProfiled_global) {
        if (loggingMRCFlags(pinfo.pidx, 0) <
            1) { //If this proc hasn't logged yet, log MRC
          if (enableLogging)
            printf("[In P%d - DONE SAMPLING]\n", pinfo.pidx);

          //Print xpoints and ypoints then interpolate to derive linear function
          arma::vec xx = arma::linspace<vec>(1, CACHE_WAYS, CACHE_WAYS);
          arma::vec yyMrc = pinfo.mrcEstimates.col(pinfo.mrcEstIndex);
          arma::vec yyIpc = pinfo.ipcCurveEstimates.col(pinfo.mrcEstIndex);

          // Need to ignore the first reading because it's only warmup period
          // Consider the second reading only
          pinfo.xPoints.at(0) = pinfo.xPoints.at(1);
          pinfo.yPoints_mpki.at(0) = pinfo.yPoints_mpki.at(1);
          pinfo.yPoints_ipc.at(0) = pinfo.yPoints_ipc.at(1);

          // Interpolate to estimate the remaining points on the curves
          interp1(pinfo.xPoints, pinfo.yPoints_mpki, xx, yyMrc, "linear");
          interp1(pinfo.xPoints, pinfo.yPoints_ipc, xx, yyIpc, "linear");

          pinfo.mrcEstimates.col(pinfo.mrcEstIndex) = yyMrc;
          pinfo.mrcEstimates.col(pinfo.mrcEstIndex)[(CACHE_WAYS - 1)] =
              pinfo.mrcEstimates.col(pinfo.mrcEstIndex)[(CACHE_WAYS - 2)];

          pinfo.ipcCurveEstimates.col(pinfo.mrcEstIndex) = yyIpc;
          pinfo.ipcCurveEstimates.col(pinfo.mrcEstIndex)[(CACHE_WAYS - 1)] =
              pinfo.ipcCurveEstimates.col(pinfo.mrcEstIndex)[(CACHE_WAYS - 2)];

          //Dump estimates to file to analyze later
          dump_mrcEstimates(pinfo);
          dump_ipcEstimates(pinfo);

          loggingMRCFlags(pinfo.pidx, 0) = 1;

          int startCol = std::max(0, (pinfo.mrcEstIndex - HIST_WINDOW_LENGTH));
          int endCol = pinfo.mrcEstIndex;
          double sum, count, avg;

          //calc avg MRC curves
          for (int w = 0; w < CACHE_WAYS; w++) {
            sum = 0.0;
            count = 0.0;
            avg = 0.0;
            for (int j = startCol; j <= endCol; j++) {
              sum += pinfo.mrcEstimates(w, j);
              count++;
            }
            avg = sum / count;
            pinfo.mrcEstAvg[w] = avg;
          }

          //calc avg IPC curves
          for (int w = 0; w < CACHE_WAYS; w++) {
            sum = 0.0;
            count = 0.0;
            avg = 0.0;
            for (int j = startCol; j <= endCol; j++) {
              sum += pinfo.ipcCurveEstimates(w, j);
              count++;
            }
            avg = sum / count;
            pinfo.ipcCurveAvg[w] = avg;
          }

          if (enableLogging) {
            printf(" ---- pinfo.mrcEstimates() ---- \n");
            pinfo.mrcEstimates
                .cols(std::max(0, (pinfo.mrcEstIndex - HIST_WINDOW_LENGTH)),
                      pinfo.mrcEstIndex).print();
            printf(" ---- pinfo.ipcCurveEstimates() ---- \n");
            pinfo.ipcCurveEstimates
                .cols(std::max(0, (pinfo.mrcEstIndex - HIST_WINDOW_LENGTH)),
                      pinfo.mrcEstIndex).print();
          } else {
            pinfo.mrcEstimates
                .cols(std::max(0, (pinfo.mrcEstIndex - HIST_WINDOW_LENGTH)),
                      pinfo.mrcEstIndex);
            pinfo.ipcCurveEstimates
                .cols(std::max(0, (pinfo.mrcEstIndex - HIST_WINDOW_LENGTH)),
                      pinfo.mrcEstIndex);
          }

          pinfo.mrcEstIndex++;

          //Store globally
          sampledMRCs.col(pinfo.pidx) = pinfo.mrcEstAvg;
          sampledIPCs.col(pinfo.pidx) = pinfo.ipcCurveAvg;

          if (enableLogging) {
            printf("\n -- sampledMRCs -- \n");
            sampledMRCs.print();

            printf("\n -- sampledIPCs -- \n");
            sampledIPCs.print();
          }

          if (procIdxProfiled_global == (NUM_CORES - 1)) {
            //Done sampling, apply partitioning
            if (enableLogging)
              printf(
                  "[Done sampling MRCs, now reapply partitioning; PHASE %d] \n",
                  pinfo.numPhases);
            stopTime("END OF PROFILING.");

            // Do cache partitioning only
            numSamples++;
            if (numSamples == numSamplesBeforePartitioning &&
                doMorePartitioning) {
              if (enableLogging)
                printf("[INFO] Clustering ... ");

              startTime();
              doClusteringForMRCs(sampledMRCs, sampledIPCs);
              stopTime("END OF CLUSTERING.");

              // Old, per-app UCP partitioning:
              //doUcpForIPCs(sampledIPCs);
              //doUcpForMRCs(sampledMRCs);

              numSamples = 0;
            }
            //[INFO]  Disable further monitoring and repartitioning
            //doMorePartitioning = false;
            //estimateMRCenabled = false;
          }

        } //end if(loggingMRCFlags(pinfo.pidx,0) < 1){  //If this proc hasn't
          //logged yet, log MRC

      } //end if( pinfo.pidx == procIdxProfiled_global )

      if (loggingMRCFlags(procIdxProfiled_global, 0) == 1) {
        if (enableLogging)
          printf("[INFO] Profiling done for PROC %d (activeProcs = %d)\n",
                 procIdxProfiled_global, activeProcs);

        loggingMRCFlags.zeros(); //= zeros<arma::vec>(NUM_CORES);
        sampleSlicesIdx = 0;     //Start over
        procIdxProfiled_global++;

        if (procIdxProfiled_global >= numProcesses) {
          procIdxProfiled_global = 0;
          monitorStartFlag = false;
        }

      }
    }          //end if( sampleSlicesIdx == numWaysToSample )
        else { //get new allocated cache ways to sample
      if (pinfo.pidx == procIdxProfiled_global) {
        if (enableLogging) {
          currentlySampling.print();
        }
        if (currentlySampling(procIdxProfiled_global, 1) ==
            0) { //profiled process done?
          if (enableLogging) {
            printf("[INFO] Master process invokes NEXT profiling plan .. \n");
          }
          arma::mat C = allAppsCacheAssignments.slice(sampleSlicesIdx);
          setCacheWaysAllCores(C, procIdxProfiled_global);
          sampleSlicesIdx++;
        } else {
          //Still some processes didn't collect IPC...
          printf("[INFO] Wait... \n");
        }
      }
    }

  }
  // --------------------------------------------------- //

  id = perf_fd2event(pinfo.fds, numEvents, info->si_fd);
  if (id == -1)
    errx(1, "cannot find event for descriptor %d", info->si_fd);
  if (id != 0)
    errx(1, "only first event is supposed to fire");

  hdr = reinterpret_cast<struct perf_event_mmap_page *>(pinfo.fds[id].buf);

  ret = perf_read_buffer(&pinfo.fds[id], &ehdr, sizeof(ehdr));
  if (ret)
    errx(1, "cannot read event header");
  if (ehdr.type != PERF_RECORD_SAMPLE) {
    errx(1, "unknown event type %d, skipping", ehdr.type);
  }

  //First val is a special case, need to read it regardless
  uint64_t dummy;
  ret = perf_read_buffer(&pinfo.fds[id], &dummy, sizeof(uint64_t));
  if (ret)
    errx(1, "cannot read first value");

  // dump_counters reads and updates values
  dump_counters(pinfo);

  if (pinfo.numPhases == pinfo.maxPhases) {
#ifdef MASTER_PROC
    // We set the limit to a really high value for everything except the
    // master proc, so we shouldn't see anything else finish before the
    // master does
    assert(pinfo.pidx == 0);
#endif
    if (--activeProcs == 0) {
      printf("[KPART] Stats collection done; killing process tree\n");
      fflush(stdout);
      inRoi = false;
      for (auto &pinfo : processInfo) {
        do {
          kill(pinfo.pid, SIGKILL);
          // ptrace(PTRACE_KILL, pinfo.pid, NULL, NULL);
        } while (waitpid(pinfo.pid, NULL, 0) != -1);
      }
    }
  }
}

void fini_handler(int sig) {
  fprintf(stdout, "[KPART] Received signal, killing process tree\n");
  fflush(stdout);
  for (auto &pinfo : processInfo) {
    do {
      kill(pinfo.pid, SIGKILL);
    } while (waitpid(pinfo.pid, NULL, 0) != -1);
    pinfo.flush();
  }
  _exit(1);
}

// Open file 'filename' with specified flags and mode, and redirect fd
// to the new file descriptor
void redirect_stream(std::string filename, int fd, int flags, int mode) {
  int newFd = open(filename.c_str(), flags, mode);
  if (newFd == -1) {
    err(-1, "Error redirecting %d to %s", fd, filename.c_str());
  }

  if (dup2(newFd, fd) == -1) {
    err(-1, "dup2 for %d failed", fd);
  }
}

std::vector<int> parse_cpuset(const cpu_set_t *cpuset) {
  std::vector<int> cores;
  int remaining = CPU_COUNT(cpuset);
  int cpu = 0;
  while (remaining) {
    if (CPU_ISSET(cpu, cpuset)) {
      --remaining;
      cores.push_back(cpu);
    }
    ++cpu;
  }

  return cores;
}

void print_core_assignments() {
  for (auto pinfo : processInfo) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    if (sched_getaffinity(pinfo.pid, sizeof(cpuset), &cpuset) == -1) {
      err(-1, "[KPART] Proc %d sched_getaffinity() failed", pinfo.pidx);
    }

    std::vector<int> cores = parse_cpuset(&cpuset);

    printf("[KPART] [Proc %d (pid %d)] Core mapping: ", pinfo.pidx, pinfo.pid);
    for (int c : cores) {
#ifdef USE_CMT
      uint64_t rmid = cmtCtrl.getRmid(c);
      printf("%d (rmid %lu) | ", c, rmid);
#else
      printf("%d | ", c);
#endif
    }
    printf("\n");
  }

  fflush(stdout);
}

void profile(char **argv) {

  for (ProcessInfo &pinfo : processInfo) {
    // Don't want buffered parent output showing up in the child's stream
    fflush(stdout);
    fflush(stderr);
    pid_t child = fork();
    if (child == -1)
      err(1, "cannot fork process\n");

    if (child == 0) {                        // child
      ptrace(PTRACE_TRACEME, 0, NULL, NULL); // pauses after exec
      char **childArgs = new char *[pinfo.args.size() + 1];

      for (int i = 0; i < pinfo.args.size(); ++i) {
        childArgs[i] = pinfo.args[i];
      }
      childArgs[pinfo.args.size()] = nullptr;

      // Per process dirs
      std::stringstream ss;
      ss << "p" << pinfo.pidx;
      if (chdir(ss.str().c_str()) == -1) {
        err(-1, "Could not chdir");
      }

      // Redirct stdin if necessary
      if (pinfo.input != "-") {
        // mode doesn't matter here since flags does not have O_CREAT
        redirect_stream(pinfo.input, STDIN_FILENO, O_RDONLY, 0);
      }

      // Redirect stdout
      redirect_stream("stdout", STDOUT_FILENO, O_WRONLY | O_CREAT | O_TRUNC,
                      S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);

      // Redirect stderr
      redirect_stream("stderr", STDERR_FILENO, O_WRONLY | O_CREAT | O_TRUNC,
                      S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);

      execvp(childArgs[0], childArgs);

      err(-1, "exec failed");
    } else { // Parent
      pinfo.pid = child;

      // Set CPU affinity for child
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);
      for (int c : pinfo.cores)
        CPU_SET(c, &cpuset);
      if (sched_setaffinity(pinfo.pid, sizeof(cpuset), &cpuset) == -1)
        err(-1, "[Proc %d] sched_setaffinity() failed", pinfo.pidx);

      printf("[KPART] Launched Process %d (pid %d)\n", pinfo.pidx, pinfo.pid);
      fflush(stdout);

      setup_counters(pinfo);
    }
  }

  print_core_assignments();

  for (ProcessInfo &pinfo : processInfo) {
#ifdef MASTER_PROC
    // Only one active process: the master
    if (pinfo.pidx == 0)
      ++activeProcs;
#else
    ++activeProcs;
#endif
    ptrace(PTRACE_CONT, pinfo.pid, NULL, NULL);
  }

#ifdef MASTER_PROC
  // Only one active process: the master
  assert(activeProcs == 1);
#endif

  inRoi = true;

  while (true) {
    int status;
    pid_t child = waitpid(-1, &status, __WALL);
    if (activeProcs == 0)
      return;
    if (child == -1) {
      if (errno == ECHILD) {
        return; // no more children
      }
    } else {
      if (WIFEXITED(status)) {
        if (inRoi) {

          printf("\n WIFEXITED(%d); inRoi=True.\n", status);
          for (auto &pinfo : processInfo) {
            printf("[EXIT-LOG] pinfo.pidx = %d, pinfo.pnumPhases = %d \n",
                   pinfo.pidx, pinfo.numPhases);
          }

          for (auto &pinfo : processInfo) {
            do {
              kill(pinfo.pid, SIGKILL);
              // ptrace(PTRACE_KILL, pinfo.pid, NULL, NULL);
            } while (waitpid(pinfo.pid, NULL, 0) != -1);
          }

          gettimeofday(&endAll, 0);
          double elapsedtime = (endAll.tv_sec - startAll.tv_sec) * 1e3 +
                               (endAll.tv_usec - startAll.tv_usec) * 1e-3;

          printf("[TIMECALC] total elapsed time = %.3f ms\n", elapsedtime);

          err(-1, "Child %d finished while in ROI (status %d)", child, status);
        } else {
          printf("[KPART] Child %d done\n", child);
          gettimeofday(&endAll, 0);
          double elapsedtime = (endAll.tv_sec - startAll.tv_sec) * 1e3 +
                               (endAll.tv_usec - startAll.tv_usec) * 1e-3;
          printf("[TIMECALC] total elapsed time = %.3f ms\n", elapsedtime);
        }
      } else {
        ptrace(PTRACE_CONT, child, NULL, NULL);
      }
    }
  }
}

std::vector<int> parse_core_list(std::string corestr) {
  std::vector<int> cores;
  size_t remaining_len = corestr.size();
  size_t begin = 0;
  size_t pos = 0;
  std::string delimiter = ",";

  while ((pos = corestr.find_first_of(delimiter, begin)) != std::string::npos) {
    size_t len = pos - begin;
    cores.push_back(stoi(corestr.substr(begin, len)));
    begin = pos + 1;
    remaining_len -= len;
  }

  if (remaining_len > 0) {
    cores.push_back(stoi(corestr.substr(begin, remaining_len)));
  }

  return cores;
}

void parse_cmdline(int argc, char **argv) {
  if (argc < 4) {
    errx(-1, "[KPART] Usage: %s <comma-sep-events> <phase_len> "
             "<logfile/- for stdout> <warmup_period_B> <profile_period_B>"
             "-- <max_phases_1> <input_redirect_1/'-' for stdin> "
             " <comma-sep-core-list> prog1 -- ...",
         argv[0]);
  }

  events = argv[1];
  phaseLen = atoll(argv[2]);

  logFile = argv[3];
  if (logFile == "-")
    prettyPrint = true;

  // Online profiling variables:
  int warmUpInstrBl = atoll(argv[4]);
  warmUpInterval = (warmUpInstrBl * 1e9) / phaseLen;

  int profilePeriodInstrBl = atoll(argv[5]);
  profileInterval = (profilePeriodInstrBl * 1e9) / phaseLen;

  invokeMonitorLen =
      warmUpInterval; //Start by invoking monitoring after a warmup period

  int arg = 5;
  numProcesses = 0;
  while (++arg < argc) {
    if (std::string(argv[arg]) == "--") {
      processInfo.push_back(ProcessInfo());
      ProcessInfo &pinfo = processInfo.back();
      int pidx = numProcesses++;
      pinfo.pidx = pidx;
#ifdef USE_CMT
      pinfo.rmid = pidx;
#endif

// Each process has, as its first three arguments
// (i.e., immediately following '--'), the following:
// max phases, input file (or '-' for stdin), a comma
//  separated list of cores it should be run on
#ifdef MASTER_PROC
      if (pidx == 0) {
        pinfo.maxPhases = atoi(argv[++arg]);
      } else {
        ++arg; // skip over arg
        pinfo.maxPhases = std::numeric_limits<int>::max();
      }
#else
      pinfo.maxPhases = atoi(argv[++arg]);
#endif
      pinfo.input = argv[++arg];
      pinfo.cores = parse_core_list(argv[++arg]);

      if (logFile == "-") {
        pinfo.logFd = stdout;
      } else {
        std::stringstream ss;
        ss << logFile << "." << pidx;
        FILE *fd = fopen(ss.str().c_str(), "w");
        if (fd == nullptr)
          errx(-1, "Error opening logFd for pidx %d", pidx);
        pinfo.logFd = fd;

        // Logging MRC estimates
        std::stringstream ssmrc;
        ssmrc << "onlineMRCSamples"
              << "." << pidx;
        FILE *mrclog = fopen(ssmrc.str().c_str(), "w");
        if (mrclog == nullptr)
          errx(-1, "Error opening log file for mrc estimates in pidx %d", pidx);
        pinfo.mrcfd = mrclog;

        // Logging IPC estimates
        std::stringstream ssipc;
        ssipc << "onlineIPCSamples"
              << "." << pidx;
        FILE *ipclog = fopen(ssipc.str().c_str(), "w");
        if (ipclog == nullptr)
          errx(-1, "Error opening log file for ipc estimates in pidx %d", pidx);
        pinfo.ipcfd = ipclog;

      }
    } else {
      processInfo.back().args.push_back(argv[arg]);
    }
  }
}

int main(int argc, char **argv) {
  gettimeofday(&startAll, 0);

  resetCacheWaysAllCores();

  //initCacheAssignSamplePlan();
  generateProfilingPlan(CACHE_WAYS);

  parse_cmdline(argc, argv);

  int ret = pfm_initialize();
  if (ret != PFM_SUCCESS)
    errx(1, "Cannot initialize library: %s", pfm_strerror(ret));

  // Set up globals
  global_setup_counters(events);

  // Print out header for logfile
  if (!prettyPrint) {
    for (ProcessInfo &pinfo : processInfo) {
      for (uint32_t i = 0; i < numEvents; i++) {
        fprintf(pinfo.logFd, "%s | %s\n", globFds[i].name, globFds[i].name);
      }
#ifdef USE_CMT
      fprintf(pinfo.logFd, "%s | %s\n", lmbName.c_str(), lmbName.c_str());
      fprintf(pinfo.logFd, "%s | %s\n", l3OccupName.c_str(),
              l3OccupName.c_str());
#endif
    }
  }

  printf("\n[KPART] Profiling %s events, logging to %s\n", events,
         (logFile == "-") ? "stdout" : logFile.c_str());

  // Instal SIGSAGE sig handler
  struct sigaction act;
  memset(&act, 0, sizeof(act));
  act.sa_sigaction = sigsage_handler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSAGE, &act, 0);

  // Ensure we kill all our children on abort
  signal(SIGSEGV, fini_handler);
  signal(SIGINT, fini_handler);
  signal(SIGABRT, fini_handler);
  signal(SIGTERM, fini_handler);

  profile(argv + 4); //skip our args

  printf("[KPART] Finished\n");

  //Teardown
  prctl(PR_TASK_PERF_EVENTS_DISABLE);
  /*for(uint32_t i = 0; i < num_fds; i++) close(fds[i].fd);
  free(fds);*/

  /* free libpfm resources cleanly */
  pfm_terminate();

  gettimeofday(&endAll, 0);
  double elapsedtime = (endAll.tv_sec - startAll.tv_sec) * 1e3 +
                       (endAll.tv_usec - startAll.tv_usec) * 1e-3;
  printf("[TIMECALC] total elapsed time = %.3f ms\n", elapsedtime);

  return ret;
}

//Set the fds
void global_setup_counters(const char *events) {
  int ret = perf_setup_list_events(events, &globFds, (int *)&numEvents);
  if (ret || !numEvents)
    errx(1, "could not initialize events: %s", pfm_strerror(ret));

  for (uint32_t i = 0; i < numEvents; i++) {
    globFds[i].hw.disabled = 0;
    globFds[i].hw.enable_on_exec = 1;
    globFds[i].hw.wakeup_events = !!i; // 0 for i=0; 1 otherwise
    globFds[i].hw.sample_type = PERF_SAMPLE_READ;
    globFds[i].hw.sample_period = (i == 0) ? phaseLen : (1L << 62);
    // pinned should only be specified for group leader
    globFds[i].hw.pinned = (i == 0) ? 1 : 0;
  }
}

void setup_counters(ProcessInfo &pinfo) {

  // NOTE: If processes are not pinned/CPUs are overcommitted, the 'pinned'
  // value for events (set in global_setup_counters()) might also need to be
  // changed
  perf_event_desc_t *fds = new perf_event_desc_t[numEvents];
  memcpy(fds, globFds, sizeof(perf_event_desc_t) * numEvents);
  for (uint32_t i = 0; i < numEvents; i++) {
    int groupFd = (i == 0) ? -1 : fds[0].fd;
    fds[i].fd = perf_event_open(&fds[i].hw, pinfo.pid, -1, groupFd, 0);
    if (fds[i].fd == -1) {
      warn("cannot attach event %s for process %d(%d)", fds[i].name, pinfo.pid,
           pinfo.pid);
    }

    assert(pidMap.find(fds[i].fd) == pidMap.end());
    pidMap[fds[i].fd] = &pinfo;

    if (i == 0) {
      int buffer_pages = 1;
      size_t pgsz = sysconf(_SC_PAGESIZE);
      fds[i].buf = mmap(NULL, (buffer_pages + 1) * pgsz, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fds[i].fd, 0);

      if (fds[i].buf == MAP_FAILED) {
        err(-1, "cannot mmap buffer");
      }

      /*
       * setup asynchronous notification on the file descriptor
       */
      int ret;
      ret = fcntl(fds[i].fd, F_SETFL, fcntl(fds[i].fd, F_GETFL, 0) | O_ASYNC);
      if (ret == -1)
        err(-1, "cannot set ASYNC");

      /*
       * necessary if we want to get the file descriptor for
       * which the SIGSAGE is sent for in siginfo->si_fd.
       * SA_SIGINFO in itself is not enough
       */
      ret = fcntl(fds[i].fd, F_SETSIG, SIGSAGE);
      if (ret == -1)
        err(-1, "cannot setsig");

      /*
       * get ownership of the descriptor
       */
      ret = fcntl(fds[i].fd, F_SETOWN, getpid());
      if (ret == -1)
        err(-1, "cannot setown");

      fds[i].pgmsk = (buffer_pages * pgsz) - 1;
    }

    // Set this really high. We decide based on counter values at run time
    // when we are finished
    int ret = ioctl(fds[i].fd, PERF_EVENT_IOC_REFRESH, 1L << 62);

    if (ret == -1)
      err(1, "cannot refresh");
  }
  pinfo.fds = fds;

#ifdef USE_CMT
  initCmt(pinfo);
#endif
}
