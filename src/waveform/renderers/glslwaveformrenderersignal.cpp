#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLContext>
#include <QWindow>
#include <QSurfaceFormat>
#include <QSurface>
#include "waveform/renderers/glslwaveformrenderersignal.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "util/cmdlineargs.h"
#include "waveform/waveform.h"
#include "waveform/waveformwidgetfactory.h"

GLSLWaveformRendererFilteredSignal::GLSLWaveformRendererFilteredSignal(WaveformWidgetRenderer* waveformWidgetRenderer)
        : WaveformRendererSignalBase(waveformWidgetRenderer),
          m_context{this}
{
    m_surface.setFormat(QSurfaceFormat::defaultFormat());
    m_surface.create();

//    m_context = new QOpenGLContext(this);
    m_context.setShareContext(QOpenGLContext::globalShareContext());
    m_context.setFormat(QSurfaceFormat::defaultFormat());
    m_context.create();
    makeCurrent();
    m_funcs = context()->versionFunctions<QOpenGLFunctions_4_3_Core>();
    gl()->initializeOpenGLFunctions();
    doneCurrent();

}
GLSLWaveformRendererFilteredSignal::~GLSLWaveformRendererFilteredSignal()
{
    makeCurrent();
    if (m_frameShaderProgram) {
        m_frameShaderProgram->release();
        m_frameShaderProgram->removeAllShaders();
    }
    delete m_frameShaderProgram;
    m_frameShaderProgram = nullptr;
    delete m_framebuffer;
    m_framebuffer = nullptr;
    gl()->glDeleteBuffers(1,&m_ssbo);
    m_ssbo = 0;
    gl()->glDeleteBuffers(1, &m_vbo);
    m_vbo = 0;
    gl()->glDeleteVertexArrays(1,&m_vao);
    m_vao = 0;
    doneCurrent();
}
QOpenGLContext *GLSLWaveformRendererFilteredSignal::context()
{
    return &m_context;
}
QSurface       *GLSLWaveformRendererFilteredSignal::surface()
{
    return &m_surface;
}
void GLSLWaveformRendererFilteredSignal::makeCurrent()
{
    if(!(m_refcnt++)) {
        if(auto _context = QOpenGLContext::currentContext()) {
            m_prev_context = _context;
            m_prev_surface = _context->surface();
        }else{
            m_prev_context = nullptr;
            m_prev_surface = nullptr;
        }
        if(m_prev_context != context()|| m_prev_surface != surface())
            context()->makeCurrent(surface());
    }
}
void GLSLWaveformRendererFilteredSignal::doneCurrent()
{
    if(!(--m_refcnt)) {
        if(m_prev_context) {
            if(m_prev_context != context() || m_prev_surface != surface())
                m_prev_context->makeCurrent(m_prev_surface);
        }else{
            context()->doneCurrent();
        }
        m_prev_context = nullptr;
        m_prev_surface = nullptr;
    }
}
bool GLSLWaveformRendererFilteredSignal::loadShaders()
{
    if(m_frameShaderProgram) {
        m_frameShaderProgram->release();
        m_frameShaderProgram->removeAllShaders();
    } else {
        m_frameShaderProgram = new QOpenGLShaderProgram(this);
    }
    qDebug() << "GLSLWaveformRendererSignal::loadShaders";
    m_shadersValid = false;
    qDebug() << "GLSLWaveformRendererSignal::loadShaders - create the program";
    do{
        auto path_prefix = QString{":/"};
        if(auto config = m_waveformRenderer->getConfig()) { path_prefix= config->getResourcePath(); }
        QDir resource_dir(path_prefix);
        resource_dir.cd("shaders");
        auto src = QString{":/shaders/passthrough.vert"};;
        if(resource_dir.exists("passthrough.vert")) {
            src = resource_dir.absoluteFilePath("passthrough.vert");
        }

        qDebug() << "GLSLWaveformRendererSignal::loadShaders - add the vertex shader";

        if (!m_frameShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, src)) {
            qDebug() << "GLSLWaveformRendererSignal::loadShaders - " << m_frameShaderProgram->log();
            break;
        }
        src = QString{":/shaders/filteredsignal.frag"};;

        if(resource_dir.exists("filteredsignal.frag")) {
            src = resource_dir.absoluteFilePath("filteredsignal.frag");
        }

        qDebug() << "GLSLWaveformRendererSignal::loadShaders - add the fragment shader";
        if (!m_frameShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, src)) {
            qDebug() << "GLSLWaveformRendererSignalShader::loadShaders - " << m_frameShaderProgram->log();
            break;
        }

        m_frameShaderProgram->bindAttributeLocation("a_position",0);

        qDebug() << "GLSLWaveformRendererSignal::loadShaders - link the program";

        if (!m_frameShaderProgram->link()) {
            qDebug() << "GLSLWaveformRendererSignal::loadShaders - " << m_frameShaderProgram->log();
            break;
        }
        return true;
    }while(false);
    delete m_frameShaderProgram;
    m_frameShaderProgram = nullptr;
    return false;
}
bool GLSLWaveformRendererFilteredSignal::loadTexture()
{
    auto trackInfo = m_waveformRenderer->getTrackInfo();
    ConstWaveformPointer waveform;
    auto dataSize = 0;
    const WaveformData* data = nullptr;

    if (trackInfo) {
        waveform = trackInfo->getWaveform();
        if (waveform) {
            dataSize = waveform->getDataSize();
            if (dataSize > 1) {
                data = waveform->data();
            }
        }
    }
    (void)gl()->glGetError();
    gl()->glActiveTexture(GL_TEXTURE0);
    if(auto err = gl()->glGetError()) {
        qDebug() << __LINE__ <<  "Error setting active texture: " << err ;
    }
    if(!m_ssbo) {
        gl()->glGenBuffers(1, &m_ssbo);
        gl()->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
    }else{
//        gl()->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
//        if(auto err = gl()->glGetError()) {
//            qDebug()<<__LINE__ << "Error binding texture : " << m_tex << " error: " << err;
//            return false;
//        }
    }
    if (waveform && data && waveform->getTextureSize()) {
        gl()->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
        gl()->glBufferData(
            GL_SHADER_STORAGE_BUFFER
          , waveform->getDataSize() *sizeof(waveform->data()[0])
          , waveform->data(),
            GL_DYNAMIC_DRAW
            );
        // Waveform ensures that getTextureSize is a multiple of
        // getTextureStride so there is no rounding here.
//        auto textureWidth  = waveform->getTextureStride();
//        auto textureHeight = waveform->getTextureSize() / waveform->getTextureStride();
//        if(m_loadedWaveform) {
//            gl()->glTexSubImage2D(GL_TEXTURE_2D, 0,0,0,textureWidth,textureHeight, GL_RGBA, GL_UNSIGNED_BYTE, data);
//        }else{
//            gl()->glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
//        }
        if(auto err = gl()->glGetError()) {
            qDebug() << "Error filling ssbo : " << m_ssbo << " error: " << err;
            return false;
        }
    }else{
          gl()->glBufferData(GL_SHADER_STORAGE_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
//        gl()->glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
//        if(auto err = gl()->glGetError()) {
//            qDebug() << "Error filling texture : " << m_tex << " error: " << err;
//            return false;
//        }
    }
    gl()->glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}
void GLSLWaveformRendererFilteredSignal::createGeometry()
{
    (void)gl()->glGetError();
    if(!m_vbo) {
        gl()->glGenBuffers(1,&m_vbo);
        if(auto err = gl()->glGetError()) {
            qDebug() << "Error generating vbo: " << err;
            m_vbo = 0;
            return;
        }else{
            qDebug() << "Created vbo: " << m_vbo;
        }
    }
    gl()->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    if(auto err = gl()->glGetError()) {
        qDebug() << "Error binding vbo: " << m_vbo << " error: " << err;
    }


    GLfloat data[] = { -1.0f, 1.0f,
                       -1.0f,-1.0f,
                        1.0f, 1.0f,
                        1.0f,-1.0f };

    gl()->glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
    if(auto err = gl()->glGetError()) {
        qDebug() << "Error filling vbo: " << m_vbo << " error: " << err;
    }else{
        qDebug() << "Filled vbo " << m_vbo << " with " << sizeof(data) << "bytes.";
    }
    if(!m_vao) {
        gl()->glGenVertexArrays(1,&m_vao);
        if(auto err = gl()->glGetError()) {
            qDebug() << "Error generating vao : " << err;
            m_vao = 0;
            return;
        }else{
            qDebug() << "Created vao: " << m_vao;
        }
    }
    gl()->glBindVertexArray(m_vao);
    if(auto err = gl()->glGetError()) {
        qDebug() << "Error binding vao: " << m_vao << " error: " << err;
    }
    gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    if(auto err = gl()->glGetError()) {
        qDebug() << "Error specifying vertex attrib pointer for vao " << m_vao << " error: " << err;
    }
    gl()->glEnableVertexAttribArray(0);
    if(auto err = gl()->glGetError()) {
        qDebug() << "Error enable attribute 0 for vao " << m_vao << " error: " << err;
    }
    gl()->glBindVertexArray(0);
    gl()->glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void GLSLWaveformRendererFilteredSignal::createFrameBuffers()
{
    auto bufferWidth = m_waveformRenderer->getWidth();
    auto bufferHeight = m_waveformRenderer->getHeight();
    auto newWidth = bufferWidth;
    auto newHeight = bufferHeight;
    if (m_framebuffer && m_framebuffer->isValid()) {
        if(m_framebuffer->width() == newWidth
        && m_framebuffer->height() == newHeight
        && m_framebuffer->isValid()) {
            return;
        }
        delete m_framebuffer;
        m_framebuffer = nullptr;
    }
    //should work with any version of OpenGl
    auto fbo_format = QOpenGLFramebufferObjectFormat{};
    fbo_format.setMipmap(false);
    fbo_format.setTextureTarget(GL_TEXTURE_2D);
    fbo_format.setTextureTarget(GL_TEXTURE_2D);
    fbo_format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    fbo_format.setInternalTextureFormat(GL_RGBA);
    m_framebuffer = new QOpenGLFramebufferObject(QSize(newWidth, newHeight), fbo_format);
    if (!m_framebuffer->isValid()) {
        qWarning() << "GLSLWaveformRendererSignal::createFrameBuffer - frame buffer not valid";
        delete m_framebuffer;
        m_framebuffer = nullptr;
    }else{
        qWarning() << "GLSLWaveformRendererSignal::createFrameBuffer - frame buffer createdsuccessfully";
    }
}

bool GLSLWaveformRendererFilteredSignal::onInit()
{
    makeCurrent();
    m_loadedWaveform = 0;
    createGeometry();
    doneCurrent();
    return true;
}
void GLSLWaveformRendererFilteredSignal::onSetup(const QDomNode& node)
{
    Q_UNUSED(node);
    makeCurrent();
    loadShaders();
    doneCurrent();
}
void GLSLWaveformRendererFilteredSignal::onSetTrack()
{
    m_loadedWaveform = 0;
    makeCurrent();
    loadTexture();
    doneCurrent();
}
void GLSLWaveformRendererFilteredSignal::onResize()
{
    makeCurrent();
    createFrameBuffers();
    doneCurrent();
}
void GLSLWaveformRendererFilteredSignal::draw(QPainter* painter, QPaintEvent* /*event*/)
{
    auto trackInfo = m_waveformRenderer->getTrackInfo();
    if (!trackInfo)
        return;
    auto waveform = trackInfo->getWaveform();
    if (waveform.isNull())
        return;
    auto dataSize = waveform->getDataSize();
    if (dataSize <= 1)
        return;
    auto data = waveform->data();
    if (!data )
        return;
    // save the GL state set for QPainter
    painter->save();
    painter->beginNativePainting();
    makeCurrent();
    if(!m_frameShaderProgram)
        loadShaders();

    if(!m_framebuffer)
        createFrameBuffers();

    if (!m_framebuffer || !m_frameShaderProgram) {
        doneCurrent();
        painter->endNativePainting();
        painter->restore();
        return;
    }
    //NOTE: (vRince) completion can change during loadTexture
    //do not remove currenCompletion temp variable !
    auto currentCompletion = waveform->getCompletion();

    if (m_loadedWaveform != currentCompletion) {
        if(loadTexture())
            m_loadedWaveform = currentCompletion;
    }else if(!m_loadedWaveform) {
        doneCurrent();
        painter->endNativePainting();
        painter->restore();
        return;
    }
    // Per-band gain from the EQ knobs.
    float lowGain(1.0), midGain(1.0), highGain(1.0), allGain(1.0);
    getGains(&allGain, &lowGain, &midGain, &highGain);

    auto firstVisualIndex = m_waveformRenderer->getFirstDisplayedPosition() * dataSize/2.0;
    auto lastVisualIndex = m_waveformRenderer->getLastDisplayedPosition() * dataSize/2.0;

    // const int firstIndex = int(firstVisualIndex+0.5);
    // firstVisualIndex = firstIndex - firstIndex%2;

    // const int lastIndex = int(lastVisualIndex+0.5);
    // lastVisualIndex = lastIndex + lastIndex%2;

    //qDebug() << "GAIN" << allGain << lowGain << midGain << highGain;

#ifndef __OPENGLES__
    //paint into frame buffer
    m_framebuffer->bind();
    gl()->glViewport(0, 0, m_framebuffer->width(), m_framebuffer->height());
//    m_framebuffer->bindDefault();
    gl()->glClearColor(0.,0.,0.,0.);
    gl()->glClear(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT );

    gl()->glUseProgram(m_frameShaderProgram->programId());

    m_frameShaderProgram->setUniformValue("firstVisualIndex", (float)firstVisualIndex);
    m_frameShaderProgram->setUniformValue("lastVisualIndex", (float)lastVisualIndex);

    m_frameShaderProgram->setUniformValue("allGain", allGain);
    m_frameShaderProgram->setUniformValue("lowGain", lowGain);
    m_frameShaderProgram->setUniformValue("midGain", midGain);
    m_frameShaderProgram->setUniformValue("highGain", highGain);
    m_frameShaderProgram->setUniformValue("framebufferSize", QVector2D(
        m_framebuffer->width(), m_framebuffer->height()));
    m_frameShaderProgram->bind();
    m_frameShaderProgram->setUniformValue("waveformLength", dataSize);
    m_frameShaderProgram->release();

    m_frameShaderProgram->setUniformValue("axesColor",
            QVector4D(m_axesColor_r, m_axesColor_g,m_axesColor_b, m_axesColor_a));

    auto lowColor  = QVector4D(m_lowColor_r, m_lowColor_g, m_lowColor_b, 1.0);
    auto midColor  = QVector4D(m_midColor_r, m_midColor_g, m_midColor_b, 1.0);
    auto highColor = QVector4D(m_highColor_r, m_highColor_g, m_highColor_b, 1.0);
    m_frameShaderProgram->setUniformValue("lowColor", lowColor);
    m_frameShaderProgram->setUniformValue("midColor", midColor);
    m_frameShaderProgram->setUniformValue("highColor", highColor);
    m_frameShaderProgram->setUniformValue("waveformDataTexture", GL_TEXTURE0);

    m_frameShaderProgram->bind();
    gl()->glBindVertexArray(m_vao);
    gl()->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    gl()->glActiveTexture(GL_TEXTURE0);
    gl()->glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,m_ssbo);

    gl()->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    gl()->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl()->glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,  0);
    gl()->glBindVertexArray(0);
    m_frameShaderProgram->release();
    gl()->glUseProgram(0);
    m_framebuffer->bindDefault();

    auto image = QImage{m_framebuffer->toImage()};
#endif
    doneCurrent();
    painter->endNativePainting();
    painter->restore();
    painter->drawImage(QRectF(0.f, 0.f, m_waveformRenderer->width(), m_waveformRenderer->height()), image);
}
