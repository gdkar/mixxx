#ifndef GLWAVEFORMRENDERERSIGNALSHADER_H
#define GLWAVEFORMRENDERERSIGNALSHADER_H

#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QtOpenGL>

#include "waveformrenderersignalbase.h"

class GLSLWaveformRendererFilteredSignal : public WaveformRendererSignalBase, public QOpenGLFunctions {
    Q_OBJECT
  public:
    explicit GLSLWaveformRendererFilteredSignal(
            WaveformWidgetRenderer* waveformWidgetRenderer);
    virtual ~GLSLWaveformRendererFilteredSignal();

    virtual bool onInit();
    virtual void onSetup(const QDomNode& node);
    virtual void draw(QPainter* painter, QPaintEvent* event);

    virtual void onSetTrack();
    virtual void onResize();

    void debugClick();
    bool loadShaders();
    bool loadTexture();
   
    bool isValid() const;
    void setFormat(QSurfaceFormat fmt);
    void makeCurrent();
    void doneCurrent();
    QOpenGLContext *context() const;
    QSurface       *surface() const;
    void destroy();
    void create();
  private:
    QSurfaceFormat  m_format{};
    QWindow         m_window;
    QOpenGLContext *m_context{nullptr};
    void createGeometry();
    void createFrameBuffers();

    GLint m_unitQuadListId;
    GLuint m_textureId;

    int m_loadedWaveform;

    //Frame buffer for two pass rendering
    bool m_frameBuffersValid;
    QOpenGLFramebufferObject* m_framebuffer;

    bool m_bDumpPng;

    // shaders
    bool m_shadersValid;
    bool m_rgbShader;
    QOpenGLShaderProgram* m_frameShaderProgram;
};

#endif // GLWAVEFORMRENDERERSIGNALSHADER_H
