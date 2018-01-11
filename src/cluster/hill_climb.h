/** $lic$
 * Copyright (C) 2012-2016 by Massachusetts Institute of Technology
 * Copyright (C) 2010-2013 by The Board of Trustees of Stanford University
 *
 * This file is part of zsim.
 *
 * zsim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * If you use this software in your research, we request that you reference
 * the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of
 * Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the
 * source of the simulator in any publications that use this software, and that
 * you send us a citation of your work. If you use code from Jigsaw
 * (src/jigsaw*, src/gmon*, src/lookahead*), we request that you reference our
 * papers ("Jigsaw: Scalable Software-Defined Caches", Beckmann and Sanchez,
 * PACT-22, September 2013 and "Scaling Distributed Cache Hierarchies through
 * Computation and Data Co-Scheduling", HPCA-21, Feb 2015). If you use code
 * from Talus (src/talus*), we request that you reference our paper ("Talus: A
 * Simple Way to Remove Cliffs in Cache Performance", Beckmann and Sanchez,
 * HPCA-21, February 2015).
 *
 * zsim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
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
