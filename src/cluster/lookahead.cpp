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
#include <numeric>
#include <queue>
#include <list>
//#include "log.h"

#include "lookahead.h"
#define NODEBUG true
#if NODEBUG
#undef info
#define info(...)
#undef warn
#define warn(...)
#endif

using namespace std;

//                   Convex Hull Lookahead

// TODO: a post-processing step in allHulls to merge convex regions to
// reduce the number of POIs. This will necessitate changes in
// bestNext and partition to support it.

namespace lookahead {

struct POI {
  int32_t x;
  double y;
  double obsolescence;
  uint32_t i;
};

std::ostream &operator<<(std::ostream &os, const POI &poi) {
  return os << "{" << poi.x << ", " << poi.y << ", " << poi.obsolescence << "}";
}

typedef std::vector<POI> POIList;

static inline bool below(const POI &prev, const POI &next, int32_t x,
                         double y) {
  double yy = prev.y + (next.y - prev.y) * (x - prev.x) / (next.x - prev.x);
  info("Prev: (%d, %g, %g) Next: (%d, %g, %g)", prev.x, prev.y,
       prev.obsolescence, next.x, next.y, next.obsolescence);
  info("%d: %g ? < %g", x, y, yy);
  return y < yy;
}
;

POIList allHulls(const u32vec &xv, const dblvec &yv) {
  const double INF = std::numeric_limits<double>::max();
  auto D = xv.n_elem;
  assert(yv.n_elem == D);

  POI start { (int32_t) xv(0), yv(0), INF, 0 }
  ;

  // all POIs, including those that aren't valid for the full hull
  // (but not any that aren't part of any partial hull)
  POIList pois { start }
  ;

  // list of non-obsolete POI (indices) that construct the current
  // hull
  int hull[D];
  hull[0] = 0;
  int hullSize = 1;

  for (int32_t i = 1; i < (int32_t) D - 1; i++) {
    int32_t x = xv(i);
    double y = yv(i);

    // Possibly a POI...add now, might remove later
    POI next { x, y, INF, (uint32_t) i }
    ;

    // should we remove our predecessor(s)?
    for (int candidate = hullSize - 1; candidate > 0; candidate--) {
      auto &cand = pois[hull[candidate]];
      auto &prev = pois[hull[candidate - 1]];

      if (!below(prev, next, cand.x, cand.y)) {
        // remove if not interesting, otherwise set obsolesence
        if (cand.x >= x - 1) {
          assert((int) pois.size() - 1 == hull[candidate]);
          pois.pop_back();
        } else {
          assert(cand.obsolescence == INF);
          cand.obsolescence = x - 1;
        }

        // either way, its not part of the hull anymore
        assert(candidate == hullSize - 1);
        --hullSize;
      } else {
        // this predecessor is legit ... so must all
        // previous, too!
        break;
      }
    }

    // this point is now part of the hull
    hull[hullSize++] = pois.size();
    pois.push_back(next);
  }

  // info("All hulls:");
  // std::cout << "{";
  // for (uint32_t d = 0; d < D; d++) {
  // auto ppoi = hull[d];
  // info("{%d, %d}", pois[ppoi].x, pois[ppoi].y);
  // std::cout << "{" << pois[ppoi].x << "," << pois[ppoi].y << "," <<
  // pois[ppoi].i << "},";
  //}
  // std::cout << "}";

  return pois;
}

POIList::iterator bestNext(POIList &pois, uint32_t size,
                           POIList::iterator current) {
  auto i = current;
  for (++i; i != pois.end(); i++) {
    if (i->x > current->x + (int32_t) size)
      break;
    if ((double) current->x + (double) size < i->obsolescence)
      return i;
  }
  // annoying case where no POI exists in [current, size]
  // concave region, so the maximum MU is at x=size
  return pois.end();
}

struct Entry {
  uint32_t partition;
  double mu;
  uint32_t alloc;
  POIList::iterator ipoi;
  bool ipoiIsEnd;

  bool operator<(const Entry &that) const { return this->mu > that.mu; }
};

std::ostream &operator<<(std::ostream &os, const Entry &entry) {
  return os << "{" << entry.partition << ", " << entry.mu << ", " << entry.alloc
            << ", " << *entry.ipoi << " }";
}

inline double at(const u32vec &xv, const dblvec &yv, uint32_t x, uint32_t i) {
  uint32_t j;
  for (j = i; j < xv.n_elem; j++) {
    if (xv(j) >= x) {
      break;
    }
  }

  if (xv(j) == x) {
    return yv(j);
  } else {
    //j = std::max(1u, std::min( j, xv.n_elem - 1));
    j = std::max((long long) 1u,
                 std::min((long long) j, (long long)(xv.n_elem - 1)));

    uint32_t x0 = xv(j - 1);
    uint32_t x1 = xv(j);
    double y0 = yv(j - 1);
    double y1 = yv(j);

    return ((x - x0) * y1 + (x1 - x) * y0) / (x1 - x0);
  }
}

void peekahead(uint32_t balance, uint32_t nparts, const u32vec xs[],
               const dblvec ys[], uint32_t *outArgAllocs) {
  info("Peekahead begin. Balance: %d parts %u", balance, nparts);

  // Initialization
  POIList::iterator current[nparts];
  POIList pois[nparts];
  std::priority_queue<Entry> queue;
  POI poiScratch[nparts];

  for (uint32_t p = 0; p < nparts; p++) {
    assert(outArgAllocs[p] == 0);
    auto &x = xs[p];
    balance -= x[0]; // minalloc
    assert(x(x.n_elem - 1) >= balance);
    pois[p] = allHulls(xs[p], ys[p]);
    assert(!pois[p].empty());
    current[p] = pois[p].begin();
  }

  auto enqueue = [&](uint32_t partition) {
    if (current[partition] == pois[partition].end())
      return;

    Entry entry;
    auto &cur = *current[partition];
    auto inext = bestNext(pois[partition], balance, current[partition]);

    if (inext != pois[partition].end()) {
      auto &next = *inext;
      entry.mu = 1. * (next.y - cur.y) / (double)(next.x - cur.x);
      entry.alloc = next.x - cur.x;
      entry.ipoiIsEnd = false;
    } else {
      auto &x = xs[partition];
      auto &y = ys[partition];

      assert(cur.y == (double) y(cur.i));
      // entry.mu = 1. * (at(x, y, cur.x + (int32_t)balance, cur.i) -
      // cur.y) / (int32_t)balance;
      // entry.alloc = balance;

      uint32_t nexti = cur.i;
      while (x[nexti] < x[cur.i] + balance)
        nexti++;
      assert(nexti < x.n_elem);

      entry.alloc = x[nexti] - x[cur.i];
      entry.mu = (y[nexti] - y[cur.i]) / (1. * entry.alloc);

      // arm:This is complicated. When inext == pois[partition].end(),
      // the contents of inext i.e inext->i, inext->x are garbage values.
      // But, these are needed in the main loop.
      // So, we have a stash of POIs(poiScratch),
      // one for each partition and inext is set to point to
      // poiScratch[partition]
      // instead of pois[partition].end(). To indicate this hack, we use
      // entry.ipoiIsEnd so that we can restore inext to
      // pois[partition].end()
      // in the main loop. But, there should be a better way to do this.
      // Also, this only works if 'i' and 'x' arrays are identical.
      entry.ipoiIsEnd = true;
      POIList::iterator it = (POIList::iterator) & (poiScratch[partition]);
      it->x = x[nexti];
      it->i = nexti;
      inext = it;
    }
    entry.ipoi = inext;
    entry.partition = partition;
    // std::cout << "Best next: " << entry << std::endl;
    queue.push(entry);
  }
  ;

  // std::cerr << "Choices for each partition (" << nparts << " total) and all
  // possible sizes (up to " << balance << ")." << std::endl;
  // for (uint32_t p = 0; p < nparts; p++) {
  //     std::cerr << "Partition: " << p << " -- ";
  //     for (uint32_t b = 0; b < balance; b++) {
  //         std::cerr << *bestNext(pois[p], b, current[p]);
  //     }
  //     std::cerr << std::endl;
  // }

  for (uint32_t p = 0; p < nparts; p++) {
    enqueue(p);
  }

  // Main loop
  while (balance > 0) {
    if (queue.empty()) {
      info("Peekahead: Queue is empty. Exiting at balance = %d", balance);
      break;
    }

    auto next = queue.top();
    // std::cerr << "Next move: " << next << "; balance: " << balance <<
    // std::endl;
    queue.pop();

    // if a partition is exhausted, balance must be exhausted as
    // well since each curve's domain > balance. if this isn't the
    // case, then we should simply only enqueue if next.ipoi !=
    // end()
    //assert_msg(current[next.partition] != pois[next.partition].end(), "WTF");
    if (balance >= next.alloc) {
      current[next.partition] =
          next.ipoiIsEnd ? pois[next.partition].end() : next.ipoi;
      if (next.mu < -0.0001) {
        info("Peekahead: Alloc %d buckets -> %d (mu %g), balance %u",
             next.alloc, next.partition, next.mu, balance);
        outArgAllocs[next.partition] = next.ipoi->i;
        assert(fabs(xs[next.partition](outArgAllocs[next.partition]) -
                    next.ipoi->x) < 1e-4);
        balance -= next.alloc;
      } else {
        info("Peekahead: No utility left. Exiting at balance = %d", balance);
        break;
      }
    } else if (next.ipoiIsEnd) {
      continue;
    }

    enqueue(next.partition);
  }
  info("Peekahead complete. Balance: %u", balance);
}

void lookahead(uint32_t balance, vector<uint32_t> minAllocs, uint32_t *allocs,
               const std::vector<const MissCurve *> &missCurves,
               bool forceZeroBalance) {
  info("Lookahead begin. Balance: %u", balance);

  auto nparts = missCurves.size();
  /*
  for (uint32_t p = 0; p < nparts; p++) {
       auto& curve = missCurves[p];
       info("Miss curve for %u :", p);
       std::stringstream mcs;
       uint32_t domSize = curve->getDomain() > 32 ? 32 : curve->getDomain();
       for (uint32_t i = 0; i < domSize; i++) {
           mcs << "(" << curve->x(i) << ", " << curve->y(i) << "), ";
       }
       info("%s", mcs.str().c_str());
  }
  */
  for (uint32_t p = 0; p < nparts; p++) {
    uint32_t minAlloc =
        minAllocs.empty() ? 0 : (minAllocs.size() > 1 ? minAllocs[p]
                                                      : minAllocs.front());
    info("Min alloc for %u is %u", p, minAlloc);
    allocs[p] = minAlloc;
    balance -= minAlloc;
  }

  // Allocs in (part, buckets) format sorted by slope
  struct CandAlloc {
    double slope;
    uint32_t part;
    uint32_t buckets;

    bool operator<(const CandAlloc &other) const {
      return slope < other.slope; /*prio queue sorts us in descending slope*/
    }
  };
  std::priority_queue<CandAlloc> candAllocs;

  auto getBestAlloc = [&](uint32_t p)->CandAlloc {
    auto curve = missCurves[p];
    double bestSlope = 0;
    uint32_t bestAlloc = 0;
    for (uint32_t s = allocs[p] + 1; s < curve->getMaxX(); s++) {
      if (s - allocs[p] > balance) {
        break; // unreachable
      }
      double benefit = (double) curve->at((uint32_t)(allocs[p]), 0) - (double)
                       curve->at(s, 0);
      double size = s - allocs[p];
      double slope = benefit / size;
      if (slope > bestSlope) {
        bestSlope = slope;
        bestAlloc = s - allocs[p];
      }
    }
    return { bestSlope, p, bestAlloc };
  }
  ;

  for (uint32_t p = 0; p < nparts; p++) {
    CandAlloc ca = getBestAlloc(p);
    if (!ca.buckets) {
      continue;
    }
    candAllocs.push(ca);
  }

  while (balance && !candAllocs.empty()) {
    CandAlloc ca = candAllocs.top();
    candAllocs.pop();
    if (ca.buckets <= balance) {
      allocs[ca.part] += ca.buckets;
      balance -= ca.buckets;
      info("Gave %u buckets to part %u (utility: %f, alloc: %u); balance: %u",
           ca.buckets, ca.part, ca.slope, allocs[ca.part], balance);
    }

    // Requeue if there is still a reachable allocation
    // Note this means if we reach an allocation with max utility that
    // exceeds the balance, we still give a chance to the partition if it
    // can produce a lower-balance alloc with a competitive utility.
    if (balance) {
      ca = getBestAlloc(ca.part);
      if (!ca.buckets) {
        continue;
      }
      candAllocs.push(ca);
    }
  }

  if (balance > 0 && forceZeroBalance) {
    warn("No utility left. Splitting evenly. balance = %d", balance);
    uint32_t activeParts = nparts;
    for (uint32_t p = 0; p < nparts; p++) {
      uint32_t nextAlloc = ((p + 1) * balance / activeParts) -
                           (p * balance / activeParts); // avoid rounding errors
      assert(nextAlloc <= balance);
      allocs[p] += nextAlloc;
      balance -= nextAlloc;
    }
    assert(balance >= 0);
    info("Terminated early with balance: %u", balance);
  }

  info("Lookahead complete. Balance: %u", balance);
  for (uint32_t p = 0; p < nparts; p++) {
    info("%u", allocs[p]);
  }
}

#define PEEKAHEAD 1

void partition(uint32_t balance, vector<uint32_t> minAllocs, uint32_t *allocs,
               const std::vector<const MissCurve *> &missCurves,
               bool forceZeroBalance) {
  if (PEEKAHEAD && !forceZeroBalance) {
    uint32_t nparts = missCurves.size();

    for (uint32_t part = 0; part < nparts; part++) {
      uint32_t minAlloc =
          minAllocs.empty() ? 0 : (minAllocs.size() > 1 ? minAllocs[part]
                                                        : minAllocs.front());
      info("Min alloc for %u is %u", part, minAlloc);
      allocs[part] = minAlloc;
      balance -= minAlloc;
    }

    /* Pre-compute number of non-skipped partitions */
    uint32_t index = 0;

    u32vec xs[nparts];
    dblvec ys[nparts];
    uint32_t indices[nparts];

    for (uint32_t part = 0; part < nparts; part++) {
      uint32_t dom =
          missCurves[part]->getDomain() - allocs[part]; // account for min alloc
      xs[index].set_size(dom);
      ys[index].set_size(dom);
      for (uint32_t i = 0; i < dom; i++) {
        assert(i + allocs[part] < missCurves[part]->getDomain());
        xs[index](i) = missCurves[part]->x(i + allocs[part]) - allocs[part];
        ys[index](i) = missCurves[part]->y(i + allocs[part]);
      }
      indices[index] = 0;
      index++;
    }

    peekahead(balance, index, xs, ys, indices);

    index = 0;
    for (uint32_t part = 0; part < nparts; part++) {
      allocs[part] += xs[index](indices[index]);
      index++;
    }

    for (uint32_t p = 0; p < nparts; p++) {
      info("%u", allocs[p]);
    }
  } else {
    lookahead(balance, minAllocs, allocs, missCurves, forceZeroBalance);
  }
}

} // namespace
