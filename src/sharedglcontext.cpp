#include <QtDebug>
#include <QGLContext>
#include <QGLFormat>
#include <QGLWidget>

#include "sharedglcontext.h"
#include <QOpenGLContext>
#include <QSurfaceFormat>
const QGLWidget* SharedGLContext::s_pSharedGLWidget = NULL;
QOpenGLContext * SharedGLContext::s_pSharedOpenGLContext = NULL;
// static
void SharedGLContext::setWidget(const QGLWidget* pWidget) {
    s_pSharedGLWidget = pWidget;
    qDebug() << "Set root GL Context widget valid:"
             << pWidget << (pWidget && pWidget->isValid());
    const QGLContext* pContext = pWidget->context();
    qDebug() << "Created root GL Context valid:" << pContext
             << (pContext && pContext->isValid());
    if (pWidget) {
        QGLFormat format = pWidget->format();
        qDebug() << "Root GL Context format:";
        qDebug() << "Double Buffering:" << format.doubleBuffer();
        qDebug() << "Swap interval:" << format.swapInterval();
        qDebug() << "Depth buffer:" << format.depth();
        qDebug() << "Direct rendering:" << format.directRendering();
        qDebug() << "Has overlay:" << format.hasOverlay();
        qDebug() << "RGBA:" << format.rgba();
        qDebug() << "Sample buffers:" << format.sampleBuffers();
        qDebug() << "Samples:" << format.samples();
        qDebug() << "Stencil buffers:" << format.stencil();
        qDebug() << "Stereo:" << format.stereo();
    }
}

// static
const QGLWidget* SharedGLContext::getWidget() {
    return s_pSharedGLWidget;
}
/* static */
QOpenGLContext* SharedGLContext::getContext(){
  if(!s_pSharedOpenGLContext){
    QSurfaceFormat format;
    format.setSamples(16);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setMajorVersion(4);
    format.setMinorVersion(5);
    QSurfaceFormat::setDefaultFormat(format);
    s_pSharedOpenGLContext = new QOpenGLContext();
    s_pSharedOpenGLContext->setFormat(format);
    s_pSharedOpenGLContext->create();
  }
  return s_pSharedOpenGLContext;
}
