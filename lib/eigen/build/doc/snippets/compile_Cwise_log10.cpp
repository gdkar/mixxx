#include <Eigen/Eigen>
#include <iostream>

using namespace Eigen;
using namespace std;

int main(int, char**)
{
  cout.precision(3);
  Array4d v(-1,0,1,2);
cout << log10(v) << endl;

  return 0;
}
