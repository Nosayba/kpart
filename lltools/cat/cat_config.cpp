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
#include <iostream>
#include <string>

#include "cat.h"
#include "cpuid.h"

using namespace std;

int main(int argc, char *argv[]) {
  cout << "=========================================" << endl;
  if (!CATController::catSupported()) {
    cout << "CAT not supported" << endl;
  } else if (!CATController::l3CatSupported()) {
    cout << "L3 CAT not supported" << endl;
  } else {
    cout << "L3 CAT config:" << endl;
    cout << "Length of capacity bit mask (CBM) = " << CATController::getCbmLen()
         << endl;
    cout << "max # of classes of service (COS) = " << CATController::getNumCos()
         << endl;
    string cdp_status = CATController::getCdpStatus() ? "ENABLED" : "DISABLED";
    cout << "CDP Status: " << cdp_status << endl;
  }
  cout << "=========================================" << endl;
  return 0;
}
