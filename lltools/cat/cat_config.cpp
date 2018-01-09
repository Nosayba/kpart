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
