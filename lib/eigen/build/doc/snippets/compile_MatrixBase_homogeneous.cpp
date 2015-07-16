#include <Eigen/Eigen>
#include <iostream>

using namespace Eigen;
using namespace std;

int main(int, char**)
{
  cout.precision(3);
  Vector3d v = Vector3d::Random(), w;
Projective3d P(Matrix4d::Random());
cout << "v                                   = [" << v.transpose() << "]^T" << endl;
cout << "h.homogeneous()                     = [" << v.homogeneous().transpose() << "]^T" << endl;
cout << "(P * v.homogeneous())               = [" << (P * v.homogeneous()).transpose() << "]^T" << endl;
cout << "(P * v.homogeneous()).hnormalized() = [" << (P * v.homogeneous()).eval().hnormalized().transpose() << "]^T" << endl;
  return 0;
}
