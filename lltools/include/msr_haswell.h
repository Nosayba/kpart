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

#define MSR_IA32_PERF_CTL (0x199)
#define MSR_IA32_PERF_STATUS (0x198)
#define MSR_PKG_ENERGY_STATUS (0x611)
#define MSR_RAPL_POWER_UNIT (0x606)
#define MSR_PKG_POWER_INFO (0x614)
#define MSR_DRAM_ENERGY_STATUS (0x619)

#define MSR_PP0_ENERGY_STATUS (0x639)
#define MSR_PP0_POWER_LIMIT (0x638)
#define MSR_PP0_PERF_STATUS (0x63B)
#define MSR_PP0_POLICY (0x63A)

#define MSR_PKG_PERF_STATUS (0x613)
#define MSR_PERF_STATUS (0x198)

#define MSR_IA32_PQR_ASSOC (0xc8f)
#define MSR_IA32_L3_MASK_0 (0xc90)

// Platform QoS MSRs (CMT)
#define MSR_IA32_QM_EVTSEL (0xc8d)
#define MSR_IA32_QM_CTR (0xc8e)
