#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <vector>

enum EHFThemeType {
  HBT_CHAMFERED,
  HBT_NINESLICE,
  HBT_CLOCKWISE
};

enum EHFTileType {
  HBT_CORNER_TOPLEFT,
  HBT_CORNER_TOPRIGHT,
  HBT_CORNER_BOTTOMRIGHT,
  HBT_CORNER_BOTTOMLEFT,
  HBT_SIDE_TOP,
  HBT_SIDE_RIGHT,
  HBT_SIDE_BUTTOM,
  HBT_SIDE_LEFT
};

class CHFTheme{
private:
  EHFThemeType themeType = HBT_CHAMFERED;
  SP<CTexture> tex = nullptr;
  SP<CTexture> norm = nullptr;

  std::vector<CBox*> tiles;
  std::vector<EHFTileType> tileType;

public:
  CHFTheme();

  ~CHFTheme() {
    if( tex && tex->m_iTexID != 0 ) 
      tex->destroyTexture();
    if( norm && norm->m_iTexID != 0 ) 
      norm->destroyTexture();

    for(uint i = 0; i < tiles.size(); i++) {
        delete tiles[i];
    }

  };
};

inline HANDLE PHANDLE = nullptr;

struct SHFGlobalState {
  CShader borderShader0;
  CShader borderShader1;
  CShader borderShader1N;

  wl_event_source* tick = nullptr;

  SP<CTexture> m_tBorderTex = makeShared<CTexture>();
  SP<CTexture> m_tBorderNorm = makeShared<CTexture>();

  std::vector<CHFTheme*> themes;
};

inline UP<SHFGlobalState> g_pGlobalState;
