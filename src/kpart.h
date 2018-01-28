/** $lic$
* MIT License
*
* Copyright (c) 2017-2018 by Massachusetts Institute of Technology
* Copyright (c) 2017-2018 by Qatar Computing Research Institute, HBKU
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
* Commodity Multicores", El-Sayed, Mukkara, Tsai, Kasture, Ma, and Sanchez,
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
**/
#pragma once

// Global config parameters for KPart.
// Set these variables according to your platform specs
const int CACHE_LINE_SIZE = 64;

// Number of historical profiling samples to average for estimating IPC-curves
// and MRC-curves
const int HIST_WINDOW_LENGTH = 3;

// Number of cores in the system
const int NUM_CORES = 8; //TODO: detect programatically

// Available cache capacity to profile and partition
const int CACHE_WAYS = 12; //TODO: detect programatically

// Paths to Intel's Cache Allocation Technology CBM and COS tools
const std::string CAT_CBM_TOOL_DIR = "../lltools/build/cat_cbm -c ";
const std::string CAT_COS_TOOL_DIR = "../lltools/build/cat_cos -c ";

//Logging, monitoring and profiling vars
const bool enableLogging(true); //Turn on for detailed logging of profiling

const bool estimateMRCenabled(true);
