#include "src/managers/input/InputManager.hpp" // g_pInputManager

#include "HFBorderDeco.hpp"
#include "HFGlobals.hpp"
#include <pango/pangocairo.h>
#include <src/desktop/Window.hpp>

// wrapper for scissor
void CHFrames::scissor(const CBox& pBox, bool transform ) {
  g_pHyprOpenGL->scissor(pBox, transform);
}

// wrapper for scissor
void CHFrames::scissor(const pixman_box32* pBox, bool transform ) {
  g_pHyprOpenGL->scissor(pBox, transform);
}

// wrapper for blend()
void CHFrames::blend(bool enabled) {
  g_pHyprOpenGL->blend(enabled);
}

// copy of hyprland src/Hyprland/src/render/OpenGl.cpp:renderBorder
void CHFrames::renderBorder(Vector2D mpos, CBox& box, const CHyprColor& color, int round, int borderSize, float a, int outerRound) {
    RASSERT((box.width > 0 && box.height > 0), "Tried to render rect with width/height < 0!");
    RASSERT(m_RenderData.pMonitor, "Tried to render rect without begin()!");

    TRACY_GPU_ZONE("RenderFBorder");

    if (m_RenderData.damage.empty() || (m_RenderData.currentWindow && m_RenderData.currentWindow->m_sWindowData.noBorder.valueOrDefault()))
        return;

    CBox newBox = box;
    m_RenderData.renderModif.applyToBox(newBox);

    if (borderSize < 1)
        return;

    int scaledBorderSize = std::round(borderSize * m_RenderData.pMonitor->scale);
    scaledBorderSize     = std::round(scaledBorderSize * m_RenderData.renderModif.combinedScale());

    // adjust box
    newBox.x -= scaledBorderSize;
    newBox.y -= scaledBorderSize;
    newBox.width += 2 * scaledBorderSize;
    newBox.height += 2 * scaledBorderSize;

    round += round == 0 ? 0 : scaledBorderSize;

    Mat3x3 matrix = m_RenderData.monitorProjection.projectBox(
        newBox, 
        wlTransformToHyprutils(
            invertTransform(!m_bEndFrame ? WL_OUTPUT_TRANSFORM_NORMAL : m_RenderData.pMonitor->transform)
        ), 
        newBox.rot);
    Mat3x3     glMatrix = m_RenderData.projection.copy().multiply(matrix);

    const auto BLEND = m_bBlend;
    blend(true); 

    CBox transformedBox = newBox;
    transformedBox.transform(
        wlTransformToHyprutils(
            invertTransform(
                m_RenderData.pMonitor->transform)),
        m_RenderData.pMonitor->vecTransformedSize.x,
        m_RenderData.pMonitor->vecTransformedSize.y);

    const auto TOPLEFT  = Vector2D(transformedBox.x, transformedBox.y);
    const auto FULLSIZE = Vector2D(transformedBox.width, transformedBox.height);

    SP<CTexture> tex = g_pGlobalState->m_tBorderTex;
    SP<CTexture> tex2 = g_pGlobalState->m_tBorderNorm;

    CShader *pShader = nullptr;
    if(m_dBorderType == 0) {
        pShader = &g_pGlobalState->borderShader0;
    }
    if(m_dBorderType == 1 && tex->m_iTexID != 0) {
      
        if ( tex->m_iTexID != 0 ) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(tex->m_iTarget, tex->m_iTexID);

            pShader = &g_pGlobalState->borderShader1;
        }
        if ( tex2->m_iTexID != 0 ) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(tex2->m_iTarget, tex2->m_iTexID);

            pShader = &g_pGlobalState->borderShader1N;
        }
    }

    glUseProgram(pShader->program);

#ifndef GLES2
    glUniformMatrix3fv(pShader->proj, 1, GL_TRUE, glMatrix.getMatrix().data());
#else
    matrixTranspose(glMatrix, glMatrix);
    glUniformMatrix3fv(pShader->proj, 1, GL_FALSE, glMatrix.getMatrix().data());

#endif

    glUniform2f(pShader->topLeft, (float)TOPLEFT.x, (float)TOPLEFT.y);
    glUniform2f(pShader->fullSize, (float)FULLSIZE.x, (float)FULLSIZE.y);
    glUniform2f(pShader->fullSizeUntransformed, (float)newBox.width, (float)newBox.height);

    glVertexAttribPointer(pShader->posAttrib, 2, GL_FLOAT, GL_FALSE, 0, fullVerts);
    glVertexAttribPointer(pShader->texAttrib, 2, GL_FLOAT, GL_FALSE, 0, fullVerts);

    glEnableVertexAttribArray(pShader->posAttrib);
    glEnableVertexAttribArray(pShader->texAttrib);


    if(m_dBorderType == 0) {
      glUniform4f(pShader->gradient, color.r, color.g, color.b, color.a);
      glUniform1f(pShader->alpha, a);

      glUniform1f(pShader->radius, round);
      glUniform1f(pShader->radiusOuter, outerRound == -1 ? round : outerRound);
      glUniform1f(pShader->thick, scaledBorderSize);
    }
    else if(m_dBorderType == 1) {
      glUniform1i(pShader->tex, 0);
      glUniform2f(pShader->radius, tex->m_vSize.x, tex->m_vSize.y);    // texSize

      // divide equally for now
      glUniform4f(pShader->radiusOuter, tex->m_vSize.x/3.0, tex->m_vSize.y/3.0, 2.0*tex->m_vSize.x/3.0, 2.0*tex->m_vSize.y/3.0);       // texSplit
  
      // normal map defined
      if ( tex2->m_iTexID != 0 ) {
        glUniform1i(pShader->alpha, 1);                                // normal map

        Vector2D mouse_coord = mpos - TOPLEFT;
        glUniform2f(pShader->angle, mouse_coord.x, mouse_coord.y);     // light position
        glUniform3f(pShader->tint, 1.0,1.0,1.0);                       // light color
        glUniform1f(pShader->applyTint, 10.0);                         // light strength
      }
    }


    if (m_RenderData.clipBox.width != 0 && m_RenderData.clipBox.height != 0) {
        CRegion damageClip{m_RenderData.clipBox.x, m_RenderData.clipBox.y, m_RenderData.clipBox.width, m_RenderData.clipBox.height};
        damageClip.intersect(m_RenderData.damage);

        if (!damageClip.empty()) {
            for (auto& RECT : damageClip.getRects()) {
                scissor(&RECT);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
    } else {
        for (auto& RECT : m_RenderData.damage.getRects()) {
            scissor(&RECT);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }

    glDisableVertexAttribArray(pShader->posAttrib);
    glDisableVertexAttribArray(pShader->texAttrib);

    if (tex->m_iTexID != 0)
        glBindTexture(tex->m_iTarget, 0);
    if (tex2->m_iTexID != 0)
        glBindTexture(tex2->m_iTarget, 0);

    blend(BLEND);
}
