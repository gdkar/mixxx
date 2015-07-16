#pragma once

#include <utility>
#include <vector>
#include <cmath>
#include <numeric>
#include <memory>
#include <Eigen/Dense>

namespace TPolyFit{
  using namespace Eigen;
  template<class T>
  void PolyFit2 ( 
      const Matrix<T,Dynamic,Dynamic> &x, 
      const Matrix<T,Dynamic,1> &y, 
      Matrix<T,Dynamic,1> &coef 
      );
  template<class T>
  void PolyFit2 (
      const vector<T>  &x,
      const vector<T>           &y,
      vector<T>                 &coef
      );
};

using namespace TPolyFit;
using namespace Eigen;
using std::vector;
template<class T>
T PolyFit2(
    const Matrix<T,Dynamic,Dynamic> &x,
    const Matrix<T,Dynamic,1>      &y,
    Matrix<T,Dynamic,1>               &coef
    )
{
  coef = x.jacobiSvd(ComputeThinU|ComputeThinV).solve(y);
}
template<class T>
  void PolyFit2(
      const vector<vector<T> > &x,
      const vector<T>          &y,
      vector<T>                &coef
      )
{
  if(!x.size())return;
  if(y.size() !=x.size())return;
  auto nrow   = x.size();
  auto ncol = coef.size();
  Matrix<T,nrow,ncol> X;
  for(auto i = 0; i < nrow;i++){
    X(i,0) = 1;
    for(auto j = 1; j < ncol; j++)
      X(i,j) = x[i]*X(i,j-1);
  }
  Matrix<T,nrow,1> Y;
  for(auto i = 0; i < nrow; i++)
    Y(i) = y[i];
  Matrix<T,ncol,1> C;
  PolyFit2(X,Y,C);
  coef.resize(ncol);
  for(auto i = 0; i < ncol;i++)
    coef[i] = C[i];
}
