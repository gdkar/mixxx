#ifndef WAVEFORM_PATH_SIMPLIFY_H
#define WAVEFORM_PATH_SIMPLIFY_H
#include <qmath.h>
#include <qsharedpointer.h>
#include <qatomic.h>
#include <QPointF>
#include <QVector2D>
#include <QLineF>
#include <QPolygonF>
#include <QVector>
#include <cmath>
#include "waveform/waveform.h"
template<typename T>
static inline T aatan2(T y, T x){
  static const T pi_25 = M_PI/4;
  static const T pi_75 = 3*M_PI/4;
  const T absy=(y<0)?-y:y;
  const T ratio=(x>=0)?((x-absy)/(x+absy)):((x+absy)/(absy-x));
  const T ratio_squared = ratio*ratio;
  const T angle = (0.1963*ratio_squared-0.9817)*ratio+((x>=0)?pi_25:pi_75);
  return (y<0)?-angle:angle;
}
static QVector<QPointF> simplify_path(const QVector<QPointF> &path,int size){
  QVector<QPointF> ret;
  ret.reserve(size+1);
  qreal L=0, T=0;
  qreal last_angle;
  for(int i=1;i<path.size();i++){
    QVector2D v(path[i]-path[i-1]);
    L+= v.length();
    qreal angle =aatan2(v.x(),v.y());
    T+= std::cbrt(std::abs(angle-last_angle));
    last_angle = angle;
  }
  const qreal fN = L*T;
  L=0;T=0,last_angle=0;
  ret.push_back( path.front());
  for(int i = 1;i<path.size() && ret.size()<=size;i++){
    QVector2D v(path[i]-path[i-1]);
    L+= v.length();
    qreal angle =aatan2(v.x(),v.y());
    T+= std::cbrt(std::abs(angle-last_angle));
    last_angle = angle;
    const qreal fi = L*T;
    if(fi>=(ret.size()*(fN/size))){
      ret.push_back(path[i]);
    }
  }
  ret.push_back(path.back());
  return ret;
}

#endif
