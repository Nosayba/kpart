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
#include "hcluster.h"
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace hcluster {

float
HCluster::missCurveAreaDistance(const std::vector<RawMissCurve> &P,
                                const std::vector<RawMissCurve> &Q) const {

  assert(P.size() == Q.size());
  auto numIntervals = P.size();
  float area = 0;
  for (uint32_t time = 0; time < numIntervals; time++) {
    auto &Pc = P[time];
    auto &Qc = Q[time];
    auto systemCurve = whirlpool::systemMissCurve(Pc, Qc);
    auto combinedCurve = whirlpool::combinedMissCurve(Pc, Qc);
    for (uint32_t i = 0; i < systemCurve.getDomain(); i++) {
      area += std::abs(combinedCurve.y(i) - systemCurve.y(i));
    }
  }
  return area / numIntervals;
}

std::vector<RawMissCurve>
HCluster::combineNodeMissCurves(const std::vector<RawMissCurve> &v1,
                                const std::vector<RawMissCurve> &v2) const {
  assert(v1.size() == v2.size());
  std::vector<RawMissCurve> newMissCurves(v1.size());
  for (uint32_t time = 0; time < newMissCurves.size(); time++) {
    newMissCurves[time] = whirlpool::combinedMissCurve(v1[time], v2[time]);
  }
  return newMissCurves;
}

//Do weighted-speedup curve partitioning
whirlpool::RawMissCurveAndBuckets HCluster::combineNodeMissCurvesDetailed(
    const std::vector<RawMissCurve> &v1,
    const std::vector<RawMissCurve> &v2) const {
  assert(v1.size() == v2.size());

  whirlpool::RawMissCurveAndBuckets rb; //result

  std::vector<RawMissCurve> newMissCurves(v1.size());
  for (uint32_t time = 0; time < newMissCurves.size(); time++) { //one time
    rb = whirlpool::combinedMissCurveDetailed(v1[time], v2[time]);
  }
  return rb;
}

void HCluster::parseCluster(std::vector<HCluster::HClusterNode *> allNodes,
                            uint32_t nodeId, int itemClusters[],
                            uint32_t numCurves, int clusterId) {
  if (nodeId >= numCurves) {
    parseCluster(allNodes, allNodes[nodeId]->left->id, itemClusters, numCurves,
                 clusterId);
    parseCluster(allNodes, allNodes[nodeId]->right->id, itemClusters, numCurves,
                 clusterId);
  } else {
    itemClusters[nodeId] = clusterId;
    return;
  }
}

void HCluster::parseClusterBuckets(
    std::vector<std::vector<std::vector<std::pair<uint32_t, uint32_t> > > > &
        clustersBucksAll,
    std::vector<HCluster::HClusterNode *> allNodes, uint32_t nodeId,
    uint32_t numCurves, int clusterId, uint32_t position, uint32_t way) {

  if (nodeId >= numCurves) {
    std::vector<std::pair<uint32_t, uint32_t> > clusterBuckList =
        allNodes[nodeId]->bucklist;
    uint32_t newposition;

    // parse left child
    if (allNodes[nodeId]->left->id >= numCurves) {
      newposition = clusterBuckList[position].first; //update bucket position
      parseClusterBuckets(clustersBucksAll, allNodes,
                          allNodes[nodeId]->left->id, numCurves, clusterId,
                          newposition, way);
    } else {
      std::pair<uint32_t, uint32_t> pr;
      pr.first = allNodes[nodeId]->left->id;
      pr.second = clusterBuckList[position].first;
      clustersBucksAll[clusterId][way].push_back(pr);
    }

    // parse right child
    if (allNodes[nodeId]->right->id >= numCurves) {
      newposition = clusterBuckList[position].second; //update bucket position
      parseClusterBuckets(clustersBucksAll, allNodes,
                          allNodes[nodeId]->right->id, numCurves, clusterId,
                          newposition, way);
    } else {
      std::pair<uint32_t, uint32_t> pr;
      pr.first = allNodes[nodeId]->right->id;
      pr.second = clusterBuckList[position].second;
      clustersBucksAll[clusterId][way].push_back(pr);
    }
  } else { // single-node cluster
           //printf("Single-node cluster corner case .. \n");
    std::pair<uint32_t, uint32_t> pr;
    pr.first = nodeId;
    pr.second = way;
    clustersBucksAll[clusterId][way].push_back(pr);
  }
  return;
}

HCluster::results_pack
HCluster::cluster(std::vector<std::vector<RawMissCurve> > curves, uint32_t K) {
  printf("[LOG] Inside HCluster::cluster() function.\n");
  uint32_t numCurves = curves.size();

  uint32_t numPointsOnCurve = curves[0][0].getDomain();
  printf("[LOG] Num points = %d.\n", numPointsOnCurve);

  results_pack results;
  std::vector<HClusterNode *> allNodes;

  std::unordered_set<uint32_t>
      activeNodes; // stores index of valid elements in 'nodes'

  for (uint32_t i = 0; i < numCurves; i++) {
    allNodes.push_back(new HClusterNode(i, curves[i]));
    //clusters.push_back(new HClusterNode(i, curves[i])); // At the beginning,
    //each item is a cluster
    activeNodes.insert(i);
  }

  typedef std::pair<uint32_t, uint32_t> nodePair;
  struct PairHash {
    std::size_t operator()(const std::pair<uint32_t, uint32_t> &p) const {
      auto h1 = std::hash<uint32_t> {}
      (p.first);
      auto h2 = std::hash<uint32_t> {}
      (p.second);
      return h1 ^ h2;
    }
  };
  std::unordered_map<nodePair, float, PairHash>
      distances; // cache of distance calculations

  std::vector<LinkageElem> linkageArray(numCurves - 1);
  auto getNumChildren = [&linkageArray, numCurves](uint32_t id) {
    if (id < numCurves) {
      return 1u;
    } else {
      return linkageArray[id - numCurves].numChildren;
    }
  }
  ;

  // main clustering loop
  uint32_t iteration = 0;
  int num_clusters = activeNodes.size();
  std::vector<HClusterNode *> clusters;
  std::unordered_set<uint32_t> nodes_in_clusters;

  int incluster[100] = { 0 };

  while (activeNodes.size() > 1) {
    float bestDistance = std::numeric_limits<float>::max();
    nodePair bestPair = std::make_pair(-1u, -1u);

    for (uint32_t src = 0; src < allNodes.size(); src++) {
      if (activeNodes.count(src)) {
        for (uint32_t dst = src + 1; dst < allNodes.size(); dst++) {
          if (activeNodes.count(dst)) {
            auto curPair = std::make_pair(src, dst);
            if (distances.count(curPair) == 0) {
              auto curDistance = missCurveAreaDistance(allNodes[src]->curves,
                                                       allNodes[dst]->curves);
              distances[curPair] = curDistance;
            }
            auto d = distances[curPair];
            if (d < bestDistance) {
              bestDistance = d;
              bestPair = curPair;
            }
          }
        }
      }
    }

    // create the new cluster
    auto src = bestPair.first;
    auto dst = bestPair.second;

    nodes_in_clusters.insert(src);
    nodes_in_clusters.insert(dst);

    incluster[src] = 1;
    incluster[dst] = 1;

    assert(src != -1u && dst != -1u);
    assert(activeNodes.count(src) && activeNodes.count(dst));

    whirlpool::RawMissCurveAndBuckets rb;
    rb = combineNodeMissCurvesDetailed(allNodes[src]->curves,
                                       allNodes[dst]->curves);

    auto newMissCurves = rb.mrcCombinedValues;
    std::vector<std::pair<uint32_t, uint32_t> > bucketsList = rb.mrcBuckets;

    linkageArray[iteration].child1 = src;
    linkageArray[iteration].child2 = dst;
    linkageArray[iteration].distance = bestDistance;
    linkageArray[iteration].numChildren =
        getNumChildren(src) + getNumChildren(dst);

    HCluster::HClusterNode *newCluster = new HClusterNode(
        allNodes.size(), allNodes[src], allNodes[dst], newMissCurves,
        bucketsList); //add list of buckets that compose the newMissCurves

    //std::cout << "Iteration " << iteration << " done; ";
    //std::cout << "Best Pair: (" << src << "," << dst << ") Best Distance: " <<
    //bestDistance << std::endl;

    // cluster ids that weren't in the original set are numbered starting from
    // numCurves
    activeNodes.erase(src);
    activeNodes.erase(dst);
    activeNodes.insert(allNodes.size());
    allNodes.push_back(newCluster);

    iteration++;
    num_clusters--;
    clusters.push_back(newCluster);

    if (num_clusters == K)
      break;
  }

  printf("\nPrinting allNodes (WITH BUCKETS) .. \n");

  for (auto node : allNodes) {
    if (node->id >= numCurves) {
      std::cout << node->id << std::endl;
      uint32_t left = node->left->id;
      uint32_t right = node->right->id;
      std::cout << left << "," << right << std::endl;

      std::cout << "Combined curve: " << node->curves[0] << std::endl;
      std::cout << "Buckets: " << std::endl;

      for (uint32_t b = 0; b < node->bucklist.size(); b++) {
        std::cout << "(" << node->bucklist[b].first << ","
                  << node->bucklist[b].second << ") ";
      }
      std::cout << std::endl;
    }
  }

  int clusterId = 0;
  int itemClusters[numCurves];
  std::vector<std::vector<RawMissCurve> > clusterCurves;

  // Checking nodes in clusters ...
  for (int h = (allNodes.size() - 1); h >= 0; h--) {
    uint32_t nodeId = allNodes[h]->id;
    if (incluster[nodeId] == 0) { //cluster found!
      clusterCurves.push_back(allNodes[nodeId]->curves);
      parseCluster(allNodes, nodeId, itemClusters, numCurves, clusterId);
      clusterId++;
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  clusterId = 0; // restart cluster ID counter
  std::vector<std::vector<std::vector<std::pair<uint32_t, uint32_t> > > >
      clustersBucksAll;
  for (int h = (allNodes.size() - 1); h >= 0; h--) {
    uint32_t nodeId = allNodes[h]->id;
    if (incluster[nodeId] == 0) { //cluster found!
      printf("Parsing buckets for cluster ID = %d .. \n", nodeId);

      std::vector<std::vector<std::pair<uint32_t, uint32_t> > > temp(
          numPointsOnCurve);
      clustersBucksAll.push_back(temp); //reserve for this cluster

      for (uint32_t way_pos = 0; way_pos < numPointsOnCurve; way_pos++) {
        parseClusterBuckets(clustersBucksAll, allNodes, nodeId, numCurves,
                            clusterId, way_pos, way_pos);
      }
      clusterId++;
    }
  }
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  printf("\nPrinting itemClusters .. \n");

  // For each item (i.e. app), ID of cluster it's a member of, and the cluster
  // combined curve
  for (uint32_t i = 0; i < numCurves; i++) {
    std::cout << "itemClusters[" << i << "]=" << itemClusters[i] << std::endl;
    int cid = itemClusters[i];
    for (uint32_t k = 0; k < clusterCurves[cid].size(); k++) {
      std::cout << clusterCurves[cid][k] << ", ";
    }
    std::cout << std::endl;
  }

  //printf("\nDeleting allNodes .. \n");
  for (auto node : allNodes) {
    delete node;
  }

  for (uint32_t i = 0; i < numCurves; i++) {
    results.item_to_clusts[i] = itemClusters[i];
  }

  results.cluster_curves = clusterCurves;
  results.cluster_buckets = clustersBucksAll;

  return results;
}

std::vector<HCluster::results_pack>
HCluster::clusterAuto(std::vector<std::vector<RawMissCurve> > curves) {
  printf("[LOG] Inside HCluster::clusterAuto() function.\n");
  uint32_t numCurves = curves.size();

  uint32_t numPointsOnCurve = curves[0][0].getDomain();
  printf("[LOG] Num points = %d.\n", numPointsOnCurve);

  results_pack results;
  std::vector<HCluster::results_pack> results_allK;
  std::vector<HClusterNode *> allNodes;

  std::unordered_set<uint32_t>
      activeNodes; // stores index of valid elements in 'nodes'

  for (uint32_t i = 0; i < numCurves; i++) {
    allNodes.push_back(new HClusterNode(i, curves[i]));
    //each item is a cluster
    activeNodes.insert(i);
  }

  typedef std::pair<uint32_t, uint32_t> nodePair;
  struct PairHash {
    std::size_t operator()(const std::pair<uint32_t, uint32_t> &p) const {
      auto h1 = std::hash<uint32_t> {}
      (p.first);
      auto h2 = std::hash<uint32_t> {}
      (p.second);
      return h1 ^ h2;
    }
  };
  std::unordered_map<nodePair, float, PairHash>
      distances; // cache of distance calculations

  std::vector<LinkageElem> linkageArray(numCurves - 1);
  auto getNumChildren = [&linkageArray, numCurves](uint32_t id) {
    if (id < numCurves) {
      return 1u;
    } else {
      return linkageArray[id - numCurves].numChildren;
    }
  }
  ;

  // main clustering loop
  uint32_t iteration = 0;
  int num_clusters = activeNodes.size();
  std::vector<HClusterNode *> clusters;
  std::unordered_set<uint32_t> nodes_in_clusters;

  int incluster[100] = { 0 };

  while (activeNodes.size() > 1) {
    printf("\n[NUM_CLUSTERS = %d]\n", (num_clusters - 1));

    float bestDistance = std::numeric_limits<float>::max();
    nodePair bestPair = std::make_pair(-1u, -1u);
    for (uint32_t src = 0; src < allNodes.size(); src++) {
      if (activeNodes.count(src)) {
        for (uint32_t dst = src + 1; dst < allNodes.size(); dst++) {
          if (activeNodes.count(dst)) {
            auto curPair = std::make_pair(src, dst);
            if (distances.count(curPair) == 0) {
              auto curDistance = missCurveAreaDistance(allNodes[src]->curves,
                                                       allNodes[dst]->curves);
              distances[curPair] = curDistance;
            }
            auto d = distances[curPair];
            if (d < bestDistance) {
              bestDistance = d;
              bestPair = curPair;
            }
          }
        }
      }
    }

    // create the new cluster
    auto src = bestPair.first;
    auto dst = bestPair.second;

    nodes_in_clusters.insert(src);
    nodes_in_clusters.insert(dst);

    incluster[src] = 1;
    incluster[dst] = 1;

    assert(src != -1u && dst != -1u);
    assert(activeNodes.count(src) && activeNodes.count(dst));

    // Return buckets so that WS curves can be computed later
    whirlpool::RawMissCurveAndBuckets rb;
    rb = combineNodeMissCurvesDetailed(allNodes[src]->curves,
                                       allNodes[dst]->curves);

    auto newMissCurves = rb.mrcCombinedValues;
    std::vector<std::pair<uint32_t, uint32_t> > bucketsList = rb.mrcBuckets;

    linkageArray[iteration].child1 = src;
    linkageArray[iteration].child2 = dst;
    linkageArray[iteration].distance = bestDistance;
    linkageArray[iteration].numChildren =
        getNumChildren(src) + getNumChildren(dst);

    HCluster::HClusterNode *newCluster = new HClusterNode(
        allNodes.size(), allNodes[src], allNodes[dst], newMissCurves,
        bucketsList); //add list of buckets that compose the newMissCurves

    // cluster ids that weren't in the original set are numbered starting from
    // numCurves
    activeNodes.erase(src);
    activeNodes.erase(dst);
    activeNodes.insert(allNodes.size());
    allNodes.push_back(newCluster);

    iteration++;
    num_clusters--;
    clusters.push_back(newCluster);

    // --------- debug printing ------------- //
    //std::cout << clusters.size() << std::endl;
    // printf("\nPrinting allNodes (WITH BUCKETS) .. \n");
    // for (auto node : allNodes) {
    //     if( node->id >= numCurves) {
    //       std::cout << node->id << std::endl;
    //       uint32_t left = node->left->id;
    //       uint32_t right = node->right->id;
    //       std::cout << left << "," << right << std::endl;
    //       std::cout << "Combined curve: " <<  node->curves[0] << std::endl;
    //       //std::cout << "Buckets: " << node->bucklist << std::endl;
    //       std::cout << "Buckets: " << std::endl;
    //       for(uint32_t b=0; b < node->bucklist.size(); b++){
    //         std::cout << "(" << node->bucklist[b].first << "," <<
    // node->bucklist[b].second << ") ";
    //       }
    //       std::cout << std::endl;
    //     }
    // }
    //std::cout << "size = " <<  allNodes.size() << std::endl;
    // --------------------------------------- //

    // New: generate clustering plans for this "num_clusters"
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
    // 1) Parse clusters/items mapping
    int clusterId = 0;
    int itemClusters[numCurves];
    std::vector<std::vector<RawMissCurve> > clusterCurves;
    // Checking nodes in clusters ...
    for (int h = (allNodes.size() - 1); h >= 0; h--) {
      uint32_t nodeId = allNodes[h]->id;
      if (incluster[nodeId] == 0) { //cluster found!
        clusterCurves.push_back(allNodes[nodeId]->curves);
        parseCluster(allNodes, nodeId, itemClusters, numCurves, clusterId);
        clusterId++;
      }
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
    // 2) find clustersBucks (the breakdown of buckets per way for each cluster)
    clusterId = 0; // restart cluster ID counter
    std::vector<std::vector<std::vector<std::pair<uint32_t, uint32_t> > > >
        clustersBucksAll;
    for (int h = (allNodes.size() - 1); h >= 0; h--) {
      uint32_t nodeId = allNodes[h]->id;
      if (incluster[nodeId] == 0) { //cluster found!
        //printf("Parsing buckets for cluster ID = %d .. \n", nodeId);
        std::vector<std::vector<std::pair<uint32_t, uint32_t> > > temp(
            numPointsOnCurve);
        clustersBucksAll.push_back(temp); //reserve for this cluster
        for (uint32_t way_pos = 0; way_pos < numPointsOnCurve; way_pos++) {
          parseClusterBuckets(clustersBucksAll, allNodes, nodeId, numCurves,
                              clusterId, way_pos, way_pos);
        }
        clusterId++;
      }
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

    // //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
    printf("\nPrinting itemClusters .. \n");
    // For each item (i.e. app), ID of cluster it's a member of, and the cluster
    // combined curve
    for (uint32_t i = 0; i < numCurves; i++) {
      std::cout << "itemClusters[" << i << "]=" << itemClusters[i] << std::endl;
      int cid = itemClusters[i];
      for (uint32_t k = 0; k < clusterCurves[cid].size(); k++) {
        std::cout << clusterCurves[cid][k];
      }
      std::cout << std::endl;
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

    // Info available here:
    // itemClusters[appIdx] => which cluster is this app member of?
    // clusterCurves[cid] => combined curve of this cluster ID
    for (uint32_t i = 0; i < numCurves; i++) {
      results.item_to_clusts[i] = itemClusters[i];
    }

    //results.item_to_clusts = itemClusters; //error
    results.cluster_curves = clusterCurves;
    results.cluster_buckets = clustersBucksAll;

    // [CHECK] scope: will results be destroyed after exiting this loop?
    results_allK.push_back(results);

  } //done parsing all possible K values ..

  return results_allK;
}

} // namespace hcluster
