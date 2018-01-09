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
#include "armadillo.h"
#include "miss_curve.h"
#include <vector>

namespace lookahead {

void peekahead(uint32_t balance, uint32_t nparts, const u32vec xs[],
               const dblvec ys[], uint32_t *outArgAllocs);

// balance - how much space there is to allocate
//
// minAllocs - minimum allocation for a partition. can be either (i)
// empty vector, which means no minimum, (ii) a vector with one entry,
// which means all partitions have the same minimum allocation, or
// (iii) a vector with a different minimum allocation per partition.
//
// allocs - output array that stores the per-partition allocations
//
// missCurves - input miss curves per partition
//
// forceZeroBalance - allocate all space even if there's no (or
// negative) benefit?
void partition(uint32_t balance, std::vector<uint32_t> minAllocs,
               uint32_t *allocs,
               const std::vector<const MissCurve *> &missCurves,
               bool forceZeroBalance = true);

} // namespace lookahead
