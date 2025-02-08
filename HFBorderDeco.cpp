#include "HFBorderDeco.hpp"

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/helpers/MiscFunctions.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <pango/pangocairo.h>

#include "HFPassElement.hpp"
#include "HFGlobals.hpp"

CHFrames::CHFrames(PHLWINDOW pWindow) : IHyprWindowDecoration(pWindow), m_pWindow(pWindow) {
    m_vLastWindowPos  = pWindow->m_vRealPosition->value();
    m_vLastWindowSize = pWindow->m_vRealSize->value();

    pTickCb = HyprlandAPI::registerCallbackDynamic(PHANDLE, "blightTick", [this](void* self, SCallbackInfo& info, std::any data) { this->onTick(); });
}

CHFrames::~CHFrames() {
    damageEntire();
    //HyprlandAPI::unregisterCallback(PHANDLE, pTickCb);
    pTickCb = nullptr;
}

SDecorationPositioningInfo CHFrames::getPositioningInfo() {
    static auto* const PBORDERS = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hframes:add_borders")->getDataStaticPtr();
    static std::vector<Hyprlang::INT* const*> PSIZES;

    for (size_t i = 0; i < 9; ++i) {
        PSIZES.push_back((Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hframes:border_size_" + std::to_string(i + 1))->getDataStaticPtr());
    }

    SDecorationPositioningInfo info;
    info.policy   = DECORATION_POSITION_STICKY;
    info.reserved = true;
    info.priority = 9990;
    info.edges    = DECORATION_EDGE_BOTTOM | DECORATION_EDGE_LEFT | DECORATION_EDGE_RIGHT | DECORATION_EDGE_TOP;

    if (m_fLastThickness == 0) {
        double size = 0;

        for (int i = 0; i < **PBORDERS; ++i) {
            size += **PSIZES[i];
        }

        info.desiredExtents = {{size, size}, {size, size}};
        m_fLastThickness    = size;
    } else {
        info.desiredExtents = {{m_fLastThickness, m_fLastThickness}, {m_fLastThickness, m_fLastThickness}};
    }

    return info;
}


void CHFrames::loadTextures() {
    // load image
    static auto* const IMG_PATH = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hframes:border_tex")->getDataStaticPtr(); 
    static auto* const NORM_PATH = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hframes:border_norm")->getDataStaticPtr(); 
    static auto* const BORDER_TYPE = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hframes:border_type")->getDataStaticPtr();
 
    loadTexture(g_pGlobalState->m_tBorderTex, *IMG_PATH);
    loadTexture(g_pGlobalState->m_tBorderNorm, *NORM_PATH);

    const auto border_type_str = std::string{*BORDER_TYPE}; 
    if (border_type_str == "ninetile" && g_pGlobalState->m_tBorderTex->m_iTexID != 0) {
        m_dBorderType = 1; 
    }
    //else if (border_type_str == "multitile")
    //    m_dBorderType = 2; 
    else {  
        m_dBorderType = 0; 
    }
}
        
void CHFrames::loadTexture(SP<CTexture> tex, const std::string path) {
    if(path.empty() || tex->m_iTexID != 0)  
      return;


    //SP<CTexture> tex = m_tBorderShape;
    const auto CAIROSURFACE = cairo_image_surface_create_from_png (absolutePath(path, g_pConfigManager->getMainConfigPath()).c_str());
    if (cairo_surface_status(CAIROSURFACE) != CAIRO_STATUS_SUCCESS) {
#ifdef DEBUG
        HyprlandAPI::addNotification(PHANDLE,
                                     "[hyprframes] Failed to load image: " +   
                                     absolutePath(path, g_pConfigManager->getMainConfigPath()),
                                     CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
#endif
        return;
    }

    // cairo to texture data
    const auto DATA = cairo_image_surface_get_data(CAIROSURFACE);

    tex->allocate();
    glBindTexture(GL_TEXTURE_2D, tex->m_iTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

#ifndef GLES2
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
#endif

    tex->m_vSize = Vector2D(
      cairo_image_surface_get_width (CAIROSURFACE),
      cairo_image_surface_get_height (CAIROSURFACE));

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        tex->m_vSize.x,
        tex->m_vSize.y,
        0, GL_RGBA, GL_UNSIGNED_BYTE, DATA);


    cairo_surface_destroy(CAIROSURFACE);

#ifdef DEBUG
    HyprlandAPI::addNotification(PHANDLE,
                                 "[hyprframes] loaded image" +   
                                 absolutePath(path, g_pConfigManager->getMainConfigPath()),
                                 CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
#endif

    return;
}

void CHFrames::onPositioningReply(const SDecorationPositioningReply& reply) {
    m_bAssignedGeometry = reply.assignedGeometry;
}

uint64_t CHFrames::getDecorationFlags() {
    return DECORATION_PART_OF_MAIN_WINDOW;
}

eDecorationLayer CHFrames::getDecorationLayer() {
    return DECORATION_LAYER_OVER;
}

std::string CHFrames::getDisplayName() {
    return "HyprFrames";
}

void CHFrames::draw(PHLMONITOR pMonitor, const float& a) {
    if (!validMapped(m_pWindow))
        return;

	  const auto PWINDOW = m_pWindow.lock();

    if(!PWINDOW->m_sWindowData.decorate.valueOrDefault())
        return;

    CHFPassElement::SHFData data;
    data.deco = this;

    g_pHyprRenderer->m_sRenderPass.add(makeShared<CHFPassElement>(data));
}

void CHFrames::drawPass(PHLMONITOR pMonitor, const float& a) {
    const auto PWINDOW = m_pWindow.lock();

    static std::vector<Hyprlang::INT* const*> PCOLORS;
    static std::vector<Hyprlang::INT* const*> PSIZES;
    for (size_t i = 0; i < 9; ++i) {
        PCOLORS.push_back((Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hframes:col.border_" + std::to_string(i + 1))->getDataStaticPtr());
        PSIZES.push_back((Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hframes:border_size_" + std::to_string(i + 1))->getDataStaticPtr());
    }
    static auto* const PBORDERS      = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hframes:add_borders")->getDataStaticPtr();
    static auto* const PBORDERSIZE   = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "general:border_size")->getDataStaticPtr();

    loadTextures();

    if (**PBORDERS < 1)
        return;

    if (m_bAssignedGeometry.width < m_seExtents.topLeft.x + 1 || m_bAssignedGeometry.height < m_seExtents.topLeft.y + 1)
        return;

    const auto PWORKSPACE      = PWINDOW->m_pWorkspace;
    const auto WORKSPACEOFFSET = PWORKSPACE && !PWINDOW->m_bPinned ? PWORKSPACE->m_vRenderOffset->value() : Vector2D();

    auto       rounding      = PWINDOW->rounding() == 0 ? 0 : (PWINDOW->rounding() + **PBORDERSIZE) * pMonitor->scale;

    CBox       fullBox = m_bAssignedGeometry;
    fullBox.translate(g_pDecorationPositioner->getEdgeDefinedPoint(DECORATION_EDGE_BOTTOM | DECORATION_EDGE_LEFT | DECORATION_EDGE_RIGHT | DECORATION_EDGE_TOP, m_pWindow.lock()));

    fullBox.translate(PWINDOW->m_vFloatingOffset - pMonitor->vecPosition + WORKSPACEOFFSET);

    if (fullBox.width < 1 || fullBox.height < 1)
        return;

    double fullThickness = 0;

   
    for (int i = 0; i < **PBORDERS; ++i) {
        const int THISBORDERSIZE = **(PSIZES[i]) == -1 ? **PBORDERSIZE : (**PSIZES[i]);
        fullThickness += THISBORDERSIZE;
    }

    fullBox.expand(-fullThickness).scale(pMonitor->scale).round();

    for (int i = 0; i < **PBORDERS; ++i) {
        const int PREVBORDERSIZESCALED = i == 0 ? 0 : (**PSIZES[i - 1] == -1 ? **PBORDERSIZE : **(PSIZES[i - 1])) * pMonitor->scale;
        const int THISBORDERSIZE       = **(PSIZES[i]) == -1 ? **PBORDERSIZE : (**PSIZES[i]);

        if (i != 0) {
            rounding += rounding == 0 ? 0 : PREVBORDERSIZESCALED;
            fullBox.x -= PREVBORDERSIZESCALED;
            fullBox.y -= PREVBORDERSIZESCALED;
            fullBox.width += PREVBORDERSIZESCALED * 2;
            fullBox.height += PREVBORDERSIZESCALED * 2;
        }

        if (fullBox.width < 1 || fullBox.height < 1)
            break;

        scissor(nullptr);

        m_bBlend = glIsEnabled(GL_BLEND);
        m_bEndFrame = false;
        m_RenderData = g_pHyprOpenGL->m_RenderData;

        renderBorder(
            g_pInputManager->getMouseCoordsInternal() - pMonitor->vecPosition,
            fullBox, 
            CHyprColor{(uint64_t) **PCOLORS[i]}, // color
            rounding, 
            THISBORDERSIZE,
            a,                                   //alpha
            -1);
    }

    m_seExtents = {{fullThickness, fullThickness}, {fullThickness, fullThickness}};

	  m_bLastRelativeBox = CBox{0, 0, m_vLastWindowSize.x, m_vLastWindowSize.y}.addExtents(m_seExtents);
        
    if (fullThickness != m_fLastThickness) {
        m_fLastThickness = fullThickness;
        g_pDecorationPositioner->repositionDeco(this);
    }

    m_bDamage = true;
}

void  CHFrames::onTick(){
    if (m_bDamage) {
        damageEntire();
        m_bDamage = false;
    }
}

eDecorationType CHFrames::getDecorationType() {
    return DECORATION_CUSTOM;
}

void CHFrames::updateWindow(PHLWINDOW pWindow) {
    m_vLastWindowPos  = pWindow->m_vRealPosition->value();
    m_vLastWindowSize = pWindow->m_vRealSize->value();

    damageEntire();
}

void CHFrames::damageEntire() {
    CBox dm = m_bLastRelativeBox.copy().translate(m_vLastWindowPos).expand(2);
    g_pHyprRenderer->damageBox(dm);
}
