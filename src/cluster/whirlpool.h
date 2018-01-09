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
#pragma once
#include "miss_curve.h"

namespace whirlpool {

RawMissCurve systemMissCurve(const MissCurve &curve1, const MissCurve &curve2);
RawMissCurve combinedMissCurve(const MissCurve &curve1,
                               const MissCurve &curve2);

struct RawMissCurveAndBuckets {
  std::vector<RawMissCurve> mrcCombinedValues; // values of the raw combined mrc
  std::vector<std::pair<uint32_t, uint32_t> >
      mrcBuckets; // for each point, breakdown of buckets among apps
};

RawMissCurveAndBuckets combinedMissCurveDetailed(const MissCurve &curve1,
                                                 const MissCurve &curve2);

}; // namespace whirlpool
