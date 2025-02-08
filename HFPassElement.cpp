#include "HFPassElement.hpp"
#include <hyprland/src/render/OpenGL.hpp>
#include "HFBorderDeco.hpp"

CHFPassElement::CHFPassElement(const CHFPassElement::SHFData& data_) : data(data_) {
    ;
}

void CHFPassElement::draw(const CRegion& damage) {
    data.deco->drawPass(g_pHyprOpenGL->m_RenderData.pMonitor.lock(), data.a);
}

bool CHFPassElement::needsLiveBlur() {
    return false;
}

bool CHFPassElement::needsPrecomputeBlur() {
    return false;
}
