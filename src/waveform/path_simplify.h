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

static QVector<QPointF>  simplify_path(const QVector<QPointF> &path,int size){
  QVector<QPointF> ret;
  QVector<double>  L(path.size());
  QVector<double>  T(path.size());
  L[0]=0;
  T[0]=0;
  qreal last_angle;
  for(int i=1;i<path.size();i++){
    QVector2D v(path[i]-path[i-1]);
    L[i] = L[i-1]+v.length();
    qreal angle = std::atan2(v.y(),v.x());
    T[i] = T[i-1]+std::cbrt(std::abs(angle-last_angle));
    last_angle = angle;
  }
  const double fN = (L.back()*T.back())/size;
  double       threshold = 0;
  for(int i = 0,k=0;i<path.size() && k<=size;i++){
    if(L[i]*T[i]>=threshold)
      ret.push_back(path[i]);
      k++;
      threshold+=fN;
  }
  return ret;
}
static QVector<QPointF>  simplify_waveform(ConstWaveformPointer&path, int size,FilterIndex part,ChannelIndex channel){
  QVector<QPointF> pts;
  float vsr = 1/(path->getVisualSampleRate());
  float yscale = 1.0/255;
  int c = channel==ChannelIndex::Left?0:1;
  switch(part){
    case FilterIndex::Low:
      for(int i=static_cast<int>(channel);i<path->getDataSize()/2;i++)
        pts.push_back(QPointF(i*vsr,path->getLow(2*i+c)*yscale));
      break;
    case FilterIndex::Mid:
      for(int i=static_cast<int>(channel);i<path->getDataSize()/2;i++)
        pts.push_back(QPointF(i*vsr,path->getMid(2*i+c)*yscale));
      break;
    case FilterIndex::High:
      for(int i=static_cast<int>(channel);i<path->getDataSize()/2;i++)
        pts.push_back(QPointF(i*vsr,path->getHigh(2*i+c)*yscale));
      break;
    default:
      for(int i=static_cast<int>(channel);i<path->getDataSize()/2;i++)
        pts.push_back(QPointF(i*vsr,path->getAll(2*i_c)*yscale));
      break;
  }
  return simplify_path(pts,size);
}
#endif
