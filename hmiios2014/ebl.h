#ifndef EBL_H
#define EBL_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QString>

#include <cmath>
#include <functional>
#include <vector>

// ---------------------------------------------------------------------------
// EblRenderContext
//
// Bundles everything the EBL (electronic bearing line) overlay needs so it can
// be drawn by a free function in this header, mirroring how MapLayerRenderContext
// decouples the generic layer renderer from TSDWindow. The GL entry points are
// supplied via `gl` (QOpenGLFunctions_3_3_Core members), and text rendering is
// injected as a callback so this header does not depend on OpenglWindow.
// ---------------------------------------------------------------------------
struct EblRenderContext
{
    QOpenGLFunctions_3_3_Core* gl = nullptr;  // raw GL entry points (must be current)
    QOpenGLShaderProgram* program = nullptr;  // shader program (must already be bound)
    GLuint colorIdUniform = 0;                // "color_id" uniform location
    GLuint posAttr = 0;                       // position attribute location
    GLuint vbo = 0;                           // EBL vertex buffer

    bool active = false;                      // true when the map op mask is EBL
    int mousePosX = 0, mousePosY = 0;         // current mouse (screen px)
    int mouseInitX = 0, mouseInitY = 0;       // press origin (screen px)
    float mouseMapX = 0.0f, mouseMapY = 0.0f; // current mouse in map coords
    bool mousePressing = false;               // left button held down
    qreal devicePixelRatio = 1.0;             // for text positioning

    std::function<void(int, int, const QString&, const QString&)> renderText;
};

// ---------------------------------------------------------------------------
// drawEbl
//
// Draws the EBL overlay: a filled circle + outline centred on (x, y) with
// radius r, plus (while pressing) a line to the current mouse and an angle /
// distance readout. The shader program must be bound by the caller; the color
// uniform is set here.
// ---------------------------------------------------------------------------
inline void drawEbl(const EblRenderContext& ctx, float x, float y, float r)
{
    constexpr int kGranularity = 63;
    QOpenGLFunctions_3_3_Core& gl = *ctx.gl;

    ctx.program->setUniformValue(ctx.colorIdUniform, 16);
    if (!ctx.active)
    {
        return;
    }

    // EBL
    std::vector<GLfloat> l_vertexBuffer((kGranularity + 1) * 2);
    GLfloat l_vertexBuffer2[4];
    int i = 0;
    for (GLdouble angle = 0; angle <= 2 * 3.1416; angle += 0.1, ++i)
    {
        l_vertexBuffer[i * 2] = (x + cos(angle) * r);
        l_vertexBuffer[i * 2 + 1] = (y - sin(angle) * r);
    }

    l_vertexBuffer[kGranularity * 2] = l_vertexBuffer[0];
    l_vertexBuffer[kGranularity * 2 + 1] = l_vertexBuffer[1];

    l_vertexBuffer2[0] = x;
    l_vertexBuffer2[1] = y;
    l_vertexBuffer2[2] = ctx.mouseMapX;
    l_vertexBuffer2[3] = ctx.mouseMapY;

    gl.glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo);
    gl.glBufferData(GL_ARRAY_BUFFER, l_vertexBuffer.size() * sizeof(GLfloat), l_vertexBuffer.data(), GL_DYNAMIC_DRAW);
    gl.glVertexAttribPointer(ctx.posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    gl.glEnableVertexAttribArray(ctx.posAttr);
    gl.glDrawArrays(GL_TRIANGLE_FAN, 0, kGranularity);
    ctx.program->setUniformValue(ctx.colorIdUniform, 3);
    gl.glDrawArrays(GL_LINE_STRIP, 0, kGranularity + 1);
    gl.glDisableVertexAttribArray(ctx.posAttr);

    float angle = 0;
    if (ctx.mousePosX != ctx.mouseInitX)
    {
        angle =
            (ctx.mousePosX - ctx.mouseInitX) >= 0
                ? atan((float)(ctx.mousePosY - ctx.mouseInitY) / (float)(ctx.mousePosX - ctx.mouseInitX)) / 3.1416 *
                      180 +
                  90
                : atan((float)(ctx.mousePosY - ctx.mouseInitY) / (float)(ctx.mousePosX - ctx.mouseInitX)) / 3.1416 *
                      180 +
                  270;
    }
    if (ctx.mousePressing)
    {
        gl.glBufferData(GL_ARRAY_BUFFER, sizeof(l_vertexBuffer2), l_vertexBuffer2, GL_DYNAMIC_DRAW);
        ctx.program->setUniformValue(ctx.colorIdUniform, 3);
        gl.glVertexAttribPointer(ctx.posAttr, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        gl.glEnableVertexAttribArray(ctx.posAttr);
        gl.glDrawArrays(GL_LINES, 0, 2);
        gl.glDisableVertexAttribArray(ctx.posAttr);

        if (ctx.renderText)
        {
            ctx.renderText(
                ctx.mousePosX * ctx.devicePixelRatio + 15, ctx.mousePosY * ctx.devicePixelRatio + 20,
                QString("Angle: %1, Dist: %2").arg(angle, 5, 'f', 1, QChar('0')).arg(r, 6, 'f', 1, QChar('0')),
                QString("Courier"));
        }
    }

    gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
}

#endif  // EBL_H
