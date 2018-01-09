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
