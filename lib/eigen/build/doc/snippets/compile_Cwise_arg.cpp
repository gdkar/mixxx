#include <Eigen/Eigen>
#include <iostream>

using namespace Eigen;
using namespace std;

int main(int, char**)
{
  cout.precision(3);
  ArrayXcf v = ArrayXcf::Random(3);
cout << v << endl << endl;
cout << arg(v) << endl;

  return 0;
}
