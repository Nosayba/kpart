#include <iostream>
#include "sysconfig.h"

using namespace std;

int main(int argc, char *argv[]) {
  vector<int> freqs = getFreqsMHz();
  cout << "Available Freqs (MHz): ";
  for (int f : freqs)
    cout << f << " ";
  cout << endl;
  return 0;
}
