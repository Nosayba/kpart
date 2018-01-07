/** $lic$
 * Copyright (C) 2015-2016 by Massachusetts Institute of Technology
 *
 * This file is part of Whirltool.
 *
 * Whirltool is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * If you use this software in your research, we request that you reference
 * the Whirlpool paper ("Whirlpool: Improving Dynamic Cache Management with Static Data
 * Classification", Mukkara, Beckmann, and Sanchez, ASPLOS-21, April 2016) as the
 * source in any publications that use this software, and that you send us a
 * citation of your work.
 *
 * Whirltool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Whirltool.  If not, see <http://www.gnu.org/licenses/>.
 *
 **/
#pragma once
#include "whirlpool.h"

namespace hcluster {

class HCluster {
private:
  std::vector<RawMissCurve>
      combineNodeMissCurves(const std::vector<RawMissCurve> &v1,
                            const std::vector<RawMissCurve> &v2) const;
  whirlpool::RawMissCurveAndBuckets
      combineNodeMissCurvesDetailed(const std::vector<RawMissCurve> &v1,
                                    const std::vector<RawMissCurve> &v2) const;
  float missCurveAreaDistance(const std::vector<RawMissCurve> &curves1,
                              const std::vector<RawMissCurve> &curves2) const;

  class HClusterNode {
  public:
    HClusterNode(uint32_t _id, const HClusterNode *_left,
                 const HClusterNode *_right,
                 const std::vector<RawMissCurve> _curves,
                 std::vector<std::pair<uint32_t, uint32_t> > _bucklist)
        : id(_id), left(_left), right(_right), curves(_curves),
          bucklist(_bucklist) {}
    HClusterNode(uint32_t _id, const std::vector<RawMissCurve> _curves)
        : id(_id), left(nullptr), right(nullptr), curves(_curves) {}
    ~HClusterNode() {}

    std::vector<uint32_t> getLeaves() const {
      std::vector<uint32_t> leaves;
      if (!left && !right) {
        leaves.push_back(id);
        return leaves;
      }
      if (left) {
        auto leftLeaves = left->getLeaves();
        leaves.insert(leaves.end(), leftLeaves.begin(), leftLeaves.end());
      }
      if (right) {
        auto rightLeaves = right->getLeaves();
        leaves.insert(leaves.end(), rightLeaves.begin(), rightLeaves.end());
      }
      return leaves;
    }

    uint32_t id;
    const HClusterNode *left;
    const HClusterNode *right;
    const std::vector<RawMissCurve> curves;
    const std::vector<std::pair<uint32_t, uint32_t> > bucklist;
  };

public:
  HCluster() {}
  ~HCluster() {}
  struct LinkageElem {
    uint32_t child1;
    uint32_t child2;
    float distance;  // distance between child1, child2 nodes when they were
                     // clustered to form a new node
    uint32_t
        numChildren; // number of original observations in the newly formed node
  };
  struct results_pack {
    int item_to_clusts[8]; //map item to cluster ID
    std::vector<std::vector<RawMissCurve> > cluster_curves;
    std::vector<std::vector<std::vector<std::pair<uint32_t, uint32_t> > > >
        cluster_buckets;
  };
  results_pack cluster(std::vector<std::vector<RawMissCurve> > curves,
                       uint32_t K);
  std::vector<HCluster::results_pack>
      clusterAuto(std::vector<std::vector<RawMissCurve> > curves);
  void parseCluster(std::vector<hcluster::HCluster::HClusterNode *> allNodes,
                    uint32_t nodeId, int itemClusters[], uint32_t numCurves,
                    int clusterId);
  void parseClusterBuckets(
      std::vector<std::vector<std::vector<std::pair<uint32_t, uint32_t> > > > &
          clustersBucksAll,
      std::vector<HCluster::HClusterNode *> allNodes, uint32_t nodeId,
      uint32_t numCurves, int clusterId, uint32_t position, uint32_t way);
};

} // namespace hcluster
