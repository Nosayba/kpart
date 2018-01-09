/*
* Copyright (C) 2017-2018 by Massachusetts Institute of Technology
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
* Commodity Multicores", El-Sayed, Mukkara, Tsai, Kasture, Ma and Sanchez,
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
*/
#include "whirlpool.h"
#include "lookahead.h"
#include <vector>

using namespace std;

namespace whirlpool {

RawMissCurve systemMissCurve(const MissCurve &curve1, const MissCurve &curve2) {
  vector<const MissCurve *> curveVec;
  curveVec.push_back(&curve1);
  curveVec.push_back(&curve2);

  vector<uint32_t> minAllocs(2, 0);

  assert(curve1.getDomain() == curve2.getDomain());
  std::vector<uint32_t> yvals(curve1.getDomain(), 0);
  yvals[0] = curve1.y(0) + curve2.y(0);
  uint32_t allocs[2];
  for (uint32_t budget = 1; budget < curve1.getDomain(); budget++) {
    lookahead::partition(budget, minAllocs, allocs, curveVec, false);
    yvals[budget] = curve1.y(allocs[0]) + curve2.y(allocs[1]);
  }
  return RawMissCurve(std::move(yvals), nullptr);
}

RawMissCurve combinedMissCurve(const MissCurve &curve1,
                               const MissCurve &curve2) {
  assert(curve1.getDomain() == curve2.getDomain());
  std::vector<uint32_t> yvals(curve1.getDomain(), 0);
  yvals[0] = curve1.y(0) + curve2.y(0);

  double indices[2];
  indices[0] = 0;
  indices[1] = 0;

  for (uint32_t p = 1; p < yvals.size(); p++) {
    double v1 = curve1.y(indices[0]);
    double v2 = curve2.y(indices[1]);

    if (v1 + v2 == 0.0) {
      v1 = 1e-10;
      v2 = 1e-10;
    }
    indices[0] += v1 / (v1 + v2);
    indices[1] += v2 / (v1 + v2);

    yvals[p] = curve1.y(indices[0]) + curve2.y(indices[1]);
  }
  return RawMissCurve(std::move(yvals), nullptr);
}

RawMissCurveAndBuckets combinedMissCurveDetailed(const MissCurve &curve1,
                                                 const MissCurve &curve2) {
  assert(curve1.getDomain() == curve2.getDomain());
  RawMissCurveAndBuckets rb; //result

  std::vector<uint32_t> yvals(curve1.getDomain(), 0);
  yvals[0] = curve1.y(0) + curve2.y(0);

  double indices[2];
  indices[0] = 0;
  indices[1] = 0;

  uint32_t buckets[2];
  buckets[0] = 0;
  buckets[1] = 0;

  std::pair<uint32_t, uint32_t> buckpair = { buckets[0], buckets[1] };
  std::vector<std::pair<uint32_t, uint32_t> > bucketsList; //vector of pairs
  bucketsList.push_back(buckpair);

  for (uint32_t p = 1; p < yvals.size(); p++) {
    double v1 = curve1.y(indices[0]);
    double v2 = curve2.y(indices[1]);
    if (v1 + v2 == 0.0) {
      v1 = 1e-10;
      v2 = 1e-10;
    }
    indices[0] += v1 / (v1 + v2);
    indices[1] += v2 / (v1 + v2);
    yvals[p] = curve1.y(indices[0]) + curve2.y(indices[1]);
    buckets[0] = (uint32_t) round(indices[0]);
    buckets[1] = p - buckets[0];
    assert(buckets[0] >= 0 && buckets[1] >= 0);
    buckpair = { buckets[0], buckets[1] };
    bucketsList.push_back(buckpair);
  }

  //std::cout << "(" << bucketsList[0].first << "," << bucketsList[0].second
  //         << ") ";
  //std::cout << "(" << bucketsList[bucketsList.size() - 1].first << ","
  //          << bucketsList[bucketsList.size() - 1].second << ") ";

  std::vector<RawMissCurve> newMissCurves; //(curve1.size());
  newMissCurves.push_back(RawMissCurve(std::move(yvals), nullptr));
  rb.mrcCombinedValues = newMissCurves;
  rb.mrcBuckets = bucketsList;

  return rb;
}

} // namespace whirlpool
