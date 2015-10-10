// libraryfeature.cpp
// Created 8/17/2009 by RJ Ryan (rryan@mit.edu)

#include "library/libraryfeature.h"

// KEEP THIS cpp file to tell scons that moc should be called on the class!!!
// The reason for this is that LibraryFeature uses slots/signals and for this
// to work the code has to be precompiles by moc
LibraryFeature::LibraryFeature(QObject* parent)
              : QObject(parent) {
}

LibraryFeature::~LibraryFeature() = default;
bool LibraryFeature::dropAccept(QList<QUrl>,QObject*)
{
  return false;
}
bool LibraryFeature::dropAcceptChild(const QModelIndex&,QList<QUrl>,QObject*)
{
  return false;
}
bool LibraryFeature::dragMoveAccept(QUrl)
{
  return false;
}
bool LibraryFeature::dragMoveAcceptChild(const QModelIndex&,QUrl)
{
  return false;
}
void LibraryFeature::bindWidget(WLibrary*,QObject*)
{
}
void LibraryFeature::activateChild(const QModelIndex&)
{
}
void LibraryFeature::onRightClick(const QPoint&)
{
}
void LibraryFeature::onRightClickChild(const QPoint&,QModelIndex)
{
}
void LibraryFeature::onLazyChildExpandation(const QModelIndex&)
{
}
