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
