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

void hillClimbingPartition(uint32_t balance, std::vector<uint32_t> minAllocs,
                           uint32_t *allocs,
                           const std::vector<const MissCurve *> &curves) {
  uint32_t nparts = curves.size();
  std::vector<double> slopes[nparts]; // Compute slopes
  for (uint32_t p = 0; p < nparts; p++) {
    slopes[p].push_back(0);
    for (uint32_t i = 0; i < curves[p]->getDomain() - 1; i++) {
      auto x0 = curves[p]->x(i);
      auto x1 = curves[p]->x(i + 1);
      auto y0 = curves[p]->y(i);
      auto y1 = curves[p]->y(i + 1);

      auto slope = 1. * (y0 - y1) / (x1 - x0);
      slopes[p].push_back(slope);
    }
  }

  uint32_t indices[nparts];
  for (uint32_t p = 0; p < nparts; p++) {
    uint32_t minAlloc =
        minAllocs.empty() ? 0 : (minAllocs.size() > 1 ? minAllocs[p]
                                                      : minAllocs.front());
    allocs[p] = minAlloc;
    balance -= minAlloc;
    indices[p] = 0;
    if (indices[p] < curves[p]->getDomain()) {
      while (allocs[p] >= curves[p]->x(indices[p])) {
        indices[p] += 1;
      }
    }
  }

  while (balance > 0) {
    double bestUtility = 0.;
    uint32_t bestPart = 0;

    for (uint32_t p = 0u; p < nparts; p++) {
      auto utility = slopes[p][indices[p]];
      if (utility > bestUtility) {
        bestPart = p;
        bestUtility = utility;
      }
    }
    allocs[bestPart] += 1;
    balance--;

    if (indices[bestPart] < curves[bestPart]->getDomain()) {
      while (allocs[bestPart] >= curves[bestPart]->x(indices[bestPart])) {
        indices[bestPart] += 1;
      }
    }
  }
}

void
hillClimbingPartitionWsCurves(uint32_t balance, std::vector<uint32_t> minAllocs,
                              uint32_t *allocs,
                              const std::vector<const MissCurve *> &curves,
                              std::vector<std::vector<double> > wsCurveVecDbl) {
  uint32_t nparts = curves.size();
  std::vector<double> slopes[nparts]; // Compute slopes

  for (uint32_t p = 0; p < nparts; p++) {
    slopes[p].push_back(0);
    std::cout << "Slopes: ";
    for (uint32_t i = 0; i < curves[p]->getDomain() - 1; i++) {
      auto x0 = curves[p]->x(i);
      auto x1 = curves[p]->x(i + 1);
      auto y0 = wsCurveVecDbl[p][i];
      auto y1 = wsCurveVecDbl[p][i + 1];

      auto slope = 1. * (y1 - y0) / (x1 - x0);
      slopes[p].push_back(slope);
      std::cout << slope << ", ";
    }
    std::cout << std::endl;
  }

  uint32_t indices[nparts];
  for (uint32_t p = 0; p < nparts; p++) {
    uint32_t minAlloc =
        minAllocs.empty() ? 0 : (minAllocs.size() > 1 ? minAllocs[p]
                                                      : minAllocs.front());
    allocs[p] = minAlloc;
    balance -= minAlloc;
    indices[p] = 0;
    if (indices[p] < curves[p]->getDomain()) {
      while (allocs[p] >= curves[p]->x(indices[p])) {
        indices[p] += 1;
      }
    }
  }

  while (balance > 0) {
    double bestUtility = 0.;
    uint32_t bestPart = 0;

    for (uint32_t p = 0u; p < nparts; p++) {
      auto utility = slopes[p][indices[p]];
      if (utility > bestUtility) {
        bestPart = p;
        bestUtility = utility;
      }
    }
    allocs[bestPart] += 1;
    balance--;

    if (indices[bestPart] < curves[bestPart]->getDomain()) {
      while (allocs[bestPart] >= curves[bestPart]->x(indices[bestPart]) &&
             indices[bestPart] < (curves[bestPart]->getDomain() - 1)) {
        indices[bestPart] += 1;
      }
    }
  }
}
