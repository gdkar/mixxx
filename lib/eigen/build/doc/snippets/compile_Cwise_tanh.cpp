#include <Eigen/Eigen>
#include <iostream>

using namespace Eigen;
using namespace std;

int main(int, char**)
{
  cout.precision(3);
  ArrayXd v = ArrayXd::LinSpaced(5,0,1);
cout << tanh(v) << endl;

  return 0;
}
