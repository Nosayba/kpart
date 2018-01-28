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
#pragma once
#include <sstream>
#include <stack>
#include "kpart.h"
#include <armadillo>
using namespace arma;
#ifdef USE_CMT
#include "cmt.h"
#endif

namespace cache_utils {

// Cache sharing-partitioning utility functions, used heavily by KPart
int share_all_cache_ways();

std::string get_cacheways_for_core(int coreIdx);

void print_allocations(uint32_t *allocs);

void apply_partition_plan(std::stack<int> partitions[]);

void do_ucp_mrcs(arma::mat mpkiVsWays);

void get_maxmarginalutil_ipcs(arma::vec curve, int cur, int parts,
                              double result[]);

void get_maxmarginalutil_mrcs(arma::vec curve, int cur, int parts,
                              double result[]);

std::vector<std::vector<double> > get_wscurves_for_combinedmrcs(
    std::vector<std::vector<std::vector<std::pair<uint32_t, uint32_t> > > >
        cluster_bucks,
    arma::mat ipcVsWays);

void smoothenIPCs(arma::mat &ipcVsWays);

void smoothenMRCs(arma::mat &mpkiVsWays);

//void verify_intel_cos_issue(std::stack<int> [] & cluster_partitions, uint32_t
//K);
void verify_intel_cos_issue(std::stack<int> *cluster_partitions, uint32_t K);

} // namespace cache_utils
