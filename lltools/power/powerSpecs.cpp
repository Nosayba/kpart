#include "energy_meter.h"
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
  EnergyMeter em;
  cout << "Package TDP: " << em.getTdp() << " Watts" << endl;
  cout << "Package Max Power: " << em.getMaxPower() << " Watts" << endl;
  cout << "Package Min Power: " << em.getMinPower() << " Watts" << endl;
  return 0;
}
