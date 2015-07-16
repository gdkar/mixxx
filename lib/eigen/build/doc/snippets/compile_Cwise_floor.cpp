#include <Eigen/Eigen>
#include <iostream>

using namespace Eigen;
using namespace std;

int main(int, char**)
{
  cout.precision(3);
  ArrayXd v = ArrayXd::LinSpaced(7,-2,2);
cout << v << endl << endl;
cout << floor(v) << endl;

  return 0;
}
