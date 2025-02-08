#pragma once

#define WLR_USE_UNSTABLE

#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>
#include <hyprland/src/render/OpenGL.hpp>


class CGradientValueData;

class CHFrames : public IHyprWindowDecoration {
  public:
    CHFrames(PHLWINDOW);
    virtual ~CHFrames();

    virtual SDecorationPositioningInfo getPositioningInfo();

    virtual void                onPositioningReply(const SDecorationPositioningReply& reply);

    virtual void                draw(PHLMONITOR, const float& a);

    virtual eDecorationType     getDecorationType();

    virtual void                updateWindow(PHLWINDOW);

    virtual void                damageEntire();

    virtual uint64_t            getDecorationFlags();

    virtual eDecorationLayer    getDecorationLayer();

    virtual std::string         getDisplayName();

  private:
    void                        drawPass(PHLMONITOR, const float& a);

    SP<HOOK_CALLBACK_FN>        pTickCb;
    void                        onTick();

    SBoxExtents                 m_seExtents;

    PHLWINDOWREF                m_pWindow;

    CBox                        m_bLastRelativeBox;
    CBox                        m_bAssignedGeometry;

    Vector2D                    m_vLastWindowPos;
    Vector2D                    m_vLastWindowSize;

    int                         m_dBorderType           = 0;

    double                      m_fLastThickness        = 0;

    void                        loadTextures();
    void                        loadTexture(SP<CTexture> tex, const std::string rawpath);

    // from OpenGL.hpp
    void                        renderBorder(Vector2D mpos, CBox&, const CHyprColor&, int round, int borderSize, float a = 1.0, int outerRound = -1 /* use round */);
    void                        scissor(const CBox&, bool transform = true);
    void                        scissor(const pixman_box32*, bool transform = true);
    void                        blend(bool enabled);
    bool                        m_bEndFrame             = false;
    bool                        m_bBlend                = false;
    SCurrentRenderData          m_RenderData;
    bool                        m_bDamage               = false;

    friend class CHFPassElement;
};
