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

QVector<QPointF>  simplify_path(const QVector<QPointF> &path,quint64 size){
  QVector<QPointF> ret;
  QVector<double>  L(path.size());
  QVector<double>  T(path.size());
  L[0]=0;
  T[0]=0;
  qreal last_angle;
  for(quint64 i=1;i<path.size();i++){
    QVector2D v(path[i]-path[i-1]);
    L[i] = L[i-1]+v.length();
    qreal angle = std::atan2(v.y(),v.x());
    T[i] = T[i-1]+std::cbrt(std::abs(angle-last_angle));
    last_angle = angle;
  }
  const double fN = (L.back()*T.back())/size;
  double       threshold = 0;
  for(quint64 i = 0,k=0;i<path.size() && k<=size;i++){
    if(L[i]*T[i]>=threshold)
      ret.push_back(path[i]);
      k++;
      threshold+=fN;
  }
  return ret;
}
QVector<QPointF>  simplify_path(ConstWaveformPointer&path, quint64 size,FilterIndex part,ChannelIndex channel){
  QVector<QPointF> pts;
  double vsr = 1/(2*path->getVisualSampleRate());
  double yscale = 1.0/256;
  switch(part){
    case FilterIndex::Low:
      for(int i=static_cast<int>(channel);i<path->getDataSize();i+=2)
        pts.push_back(QPointF(i*vsr,path->getLow(i)*yscale));
      break;
    case FilterIndex::Mid:
      for(int i=static_cast<int>(channel);i<path->getDataSize();i+=2)
        pts.push_back(QPointF(i*vsr,path->getMid(i)*yscale));
      break;
    case FilterIndex::High:
      for(int i=static_cast<int>(channel);i<path->getDataSize();i+=2)
        pts.push_back(QPointF(i*vsr,path->getHigh(i)*yscale));
      break;
    default:
      for(int i=static_cast<int>(channel);i<path->getDataSize();i+=2)
        pts.push_back(QPointF(i*vsr,path->getAll(i)*yscale));
      break;
  }
  return simplify_path(pts,size);
}
#endif
