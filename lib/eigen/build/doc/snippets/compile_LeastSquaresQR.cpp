#include <Eigen/Eigen>
#include <iostream>

using namespace Eigen;
using namespace std;

int main(int, char**)
{
  cout.precision(3);
  MatrixXf A = MatrixXf::Random(3, 2);
VectorXf b = VectorXf::Random(3);
cout << "The solution using the QR decomposition is:\n"
     << A.colPivHouseholderQr().solve(b) << endl;

  return 0;
}
