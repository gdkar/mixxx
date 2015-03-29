#include <QtDebug>
#include <QOpenGLContext>
#include <QGLFormat>
#include <QOpenGLContext>

#include "sharedglcontext.h"

QOpenGLContext * SharedGLContext::s_pSharedGLContext= NULL;

// static
void SharedGLContext::setWidget(QOpenGLContext * pWidget) {
    s_pSharedGLContext = pWidget;
    qDebug() << "Set root GL Context widget valid:"
             << pWidget << (pWidget && pWidget->isValid());
    QOpenGLContext* pContext = pWidget;
    qDebug() << "Created root GL Context valid:" << pContext
             << (pContext && pContext->isValid());
//    if (pWidget) {
//        QGLSurfaceFormat format = pWidget->format();
/*        qDebug() << "Root GL Context format:";
        qDebug() << "Double Buffering:" << format.doubleBuffer();
        qDebug() << "Swap interval:" << format.swapInterval();
        qDebug() << "Depth buffer:" << format.depth();
        qDebug() << "Direct rendering:" << format.directRendering();
        qDebug() << "Has overlay:" << format.hasOverlay();
        qDebug() << "RGBA:" << format.rgba();
        qDebug() << "Sample buffers:" << format.sampleBuffers();
        qDebug() << "Samples:" << format.samples();
        qDebug() << "Stencil buffers:" << format.stencil();
        qDebug() << "Stereo:" << format.stereo();*/
//    }
}

// static
QOpenGLContext * SharedGLContext::getWidget() {
    return s_pSharedGLContext;
}
