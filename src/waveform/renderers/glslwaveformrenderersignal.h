#ifndef GLWAVEFORMRENDERERSIGNALSHADER_H
#define GLWAVEFORMRENDERERSIGNALSHADER_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QtOpenGL>

#include "waveformrenderersignalbase.h"
#include "waveform/renderers/waveformwidgetrenderer.h"

class GLSLWaveformRendererFilteredSignal : public WaveformRendererSignalBase {
    Q_OBJECT
  public:
    Q_INVOKABLE explicit GLSLWaveformRendererFilteredSignal(
            WaveformWidgetRenderer* waveformWidgetRenderer);
    virtual ~GLSLWaveformRendererFilteredSignal();

    virtual bool onInit();
    virtual void onSetup(const QDomNode& node);
    virtual void draw(QPainter* painter, QPaintEvent* event);

    virtual void onSetTrack();
    virtual void onResize();
    bool loadShaders();
    bool loadTexture();
    void createGeometry();
    void createFrameBuffers();

    bool isValid() const;
    void makeCurrent();
    void doneCurrent();
    QOpenGLContext *context();
    QSurface       *surface();
    QOpenGLFunctions_3_3_Core *gl() const { return m_funcs;}
  private:
    QOffscreenSurface m_surface{};
    QOpenGLContext    m_context{};
    QOpenGLFunctions_3_3_Core *m_funcs{nullptr};
    UserSettingsPointer m_pConfig{nullptr};

    QOpenGLFramebufferObject* m_framebuffer{nullptr};
    QOpenGLShaderProgram* m_frameShaderProgram{nullptr};

    int              m_refcnt{0};
    QSurface         *m_prev_surface{nullptr};
    QOpenGLContext   *m_prev_context{nullptr};


    GLuint m_vao{0};
    GLuint m_vbo{0};
    GLuint m_tex{0};

    int m_loadedWaveform{0};

    //Frame buffer for two pass rendering
    bool m_frameBuffersValid{false};
    bool m_shadersValid{false};
    // shaders
};

#endif // GLWAVEFORMRENDERERSIGNALSHADER_H
