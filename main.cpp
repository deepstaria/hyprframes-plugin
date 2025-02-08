#define WLR_USE_UNSTABLE

#include <unistd.h>

#include <any>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/managers/HookSystemManager.hpp>

#include "HFBorderDeco.hpp"
#include "HFGlobals.hpp"
#include "shaders/Border.hpp"
#include "shaders/Textures.hpp"

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

void onNewWindow(void* self, std::any data) {
    // data is guaranteed
    const auto PWINDOW = std::any_cast<PHLWINDOW>(data);

    HyprlandAPI::addWindowDecoration(PHANDLE, PWINDOW, makeUnique<CHFrames>(PWINDOW));
}

GLuint CompileShader(const GLuint& type, std::string src) {
    auto shader = glCreateShader(type);

    auto shaderSource = src.c_str();

    glShaderSource(shader, 1, (const GLchar**)&shaderSource, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

    if (ok == GL_FALSE)
        throw std::runtime_error("CompileShader() failed! " + src);

    return shader;
}

GLuint CreateProgram(const std::string& vert, const std::string& frag) {
    auto vertCompiled = CompileShader(GL_VERTEX_SHADER, vert);

    if (!vertCompiled)
        throw std::runtime_error("Compiling vshader failed.");

    auto fragCompiled = CompileShader(GL_FRAGMENT_SHADER, frag);

    if (!fragCompiled)
        throw std::runtime_error("Compiling fshader failed.");

    auto prog = glCreateProgram();
    glAttachShader(prog, vertCompiled);
    glAttachShader(prog, fragCompiled);
    glLinkProgram(prog);

    glDetachShader(prog, vertCompiled);
    glDetachShader(prog, fragCompiled);
    glDeleteShader(vertCompiled);
    glDeleteShader(fragCompiled);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);

    if (ok == GL_FALSE)
        throw std::runtime_error("CreateProgram() failed! GL_LINK_STATUS not OK!");

    return prog;
}

int onTick(void* data) {
    EMIT_HOOK_EVENT("HFTick", nullptr);

    const int TIMEOUT = g_pHyprRenderer->m_pMostHzMonitor ? 1000.0 / g_pHyprRenderer->m_pMostHzMonitor->refreshRate : 16;
    wl_event_source_timer_update(g_pGlobalState->tick, TIMEOUT);

    return 0;
}


void initGlobal() {
    g_pHyprRenderer->makeEGLCurrent();

    // compile shaders
    GLuint prog                                = CreateProgram(QUADVERTSRC, FRAG_HB_CHAM);
    g_pGlobalState->borderShader0.program      = prog;
    g_pGlobalState->borderShader0.proj         = glGetUniformLocation(prog, "proj");
    g_pGlobalState->borderShader0.posAttrib    = glGetAttribLocation(prog, "pos");
    g_pGlobalState->borderShader0.texAttrib    = glGetAttribLocation(prog, "texcoord");

    g_pGlobalState->borderShader0.topLeft      = glGetUniformLocation(prog, "topLeft");
    g_pGlobalState->borderShader0.bottomRight  = glGetUniformLocation(prog, "bottomRight");
    g_pGlobalState->borderShader0.fullSize     = glGetUniformLocation(prog, "fullSize");
    g_pGlobalState->borderShader0.fullSizeUntransformed = glGetUniformLocation(prog, "fullSizeUntransformed");
    g_pGlobalState->borderShader0.radius       = glGetUniformLocation(prog, "radius");
    g_pGlobalState->borderShader0.radiusOuter  = glGetUniformLocation(prog, "radiusOuter");
    g_pGlobalState->borderShader0.thick        = glGetUniformLocation(prog, "thick");

    g_pGlobalState->borderShader0.gradient     = glGetUniformLocation(prog, "f_color");
    g_pGlobalState->borderShader0.alpha        = glGetUniformLocation(prog, "alpha");

    prog                                       = CreateProgram(QUADVERTSRC, FRAG_HB_IMG9);
    g_pGlobalState->borderShader1.program      = prog;
    g_pGlobalState->borderShader1.proj         = glGetUniformLocation(prog, "proj");
    g_pGlobalState->borderShader1.posAttrib    = glGetAttribLocation(prog, "pos");
    g_pGlobalState->borderShader1.texAttrib    = glGetAttribLocation(prog, "texcoord");

    g_pGlobalState->borderShader1.tex          = glGetUniformLocation(prog, "tex");
    g_pGlobalState->borderShader1.radius       = glGetUniformLocation(prog, "texSize");
    g_pGlobalState->borderShader1.radiusOuter  = glGetUniformLocation(prog, "texSplit");

    g_pGlobalState->borderShader1.topLeft      = glGetUniformLocation(prog, "topLeft");
    g_pGlobalState->borderShader1.fullSize     = glGetUniformLocation(prog, "fullSize");
    g_pGlobalState->borderShader1.fullSizeUntransformed = glGetUniformLocation(prog, "fullSizeUntransformed");

    prog                                       = CreateProgram(QUADVERTSRC, FRAG_HB_IMG9N);
    g_pGlobalState->borderShader1N.program     = prog;
    g_pGlobalState->borderShader1N.proj        = glGetUniformLocation(prog, "proj");
    g_pGlobalState->borderShader1N.posAttrib   = glGetAttribLocation(prog, "pos");
    g_pGlobalState->borderShader1N.texAttrib   = glGetAttribLocation(prog, "texcoord");

    g_pGlobalState->borderShader1N.tex         = glGetUniformLocation(prog, "tex");
    g_pGlobalState->borderShader1N.alpha       = glGetUniformLocation(prog, "normalMap");
    g_pGlobalState->borderShader1N.radius      = glGetUniformLocation(prog, "texSize");
    g_pGlobalState->borderShader1N.radiusOuter = glGetUniformLocation(prog, "texSplit");

    g_pGlobalState->borderShader1N.tint        = glGetUniformLocation(prog, "lightColor");
    g_pGlobalState->borderShader1N.applyTint   = glGetUniformLocation(prog, "lightIntensity");
    g_pGlobalState->borderShader1N.angle       = glGetUniformLocation(prog, "lightPos");

    g_pGlobalState->borderShader1N.topLeft     = glGetUniformLocation(prog, "topLeft");
    g_pGlobalState->borderShader1N.fullSize    = glGetUniformLocation(prog, "fullSize");
    g_pGlobalState->borderShader1N.fullSizeUntransformed = glGetUniformLocation(prog, "fullSizeUntransformed");

    g_pGlobalState->tick = wl_event_loop_add_timer(g_pCompositor->m_sWLEventLoop, &onTick, nullptr);
    wl_event_source_timer_update(g_pGlobalState->tick, 1);
}

static void onPreConfigReload() {
    for(uint i = 0; i < g_pGlobalState->themes.size(); i++) {
        delete g_pGlobalState->themes[i];
    }

    g_pGlobalState->themes.clear();
}

Hyprlang::CParseResult onNewTheme(const char* K, const char* V) {
    Hyprlang::CParseResult result;

    return result;
}

Hyprlang::CParseResult onAddTex(const char* K, const char* V) {
    Hyprlang::CParseResult result;

    return result;
}

Hyprlang::CParseResult onAddNorm(const char* K, const char* V) {
    Hyprlang::CParseResult result;

    return result;
} 

Hyprlang::CParseResult onAddTile(const char* K, const char* V) {
    Hyprlang::CParseResult result;

    return result;
}

Hyprlang::CParseResult onSetClass(const char* K, const char* V) {
    Hyprlang::CParseResult result;

    return result;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH = __hyprland_api_get_hash();

    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[hyprframes] Failure in initialization: Version mismatch (headers ver is not equal to running hyprland ver)",
                                     CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[fb] Version mismatch");
    }

    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hframes:add_borders", Hyprlang::INT{1});

    for (size_t i = 0; i < 9; ++i) {
        HyprlandAPI::addConfigValue(PHANDLE, "plugin:hframes:col.border_" + std::to_string(i + 1), Hyprlang::INT{*configStringToInt("rgba(000000ee)")}); 
        HyprlandAPI::addConfigValue(PHANDLE, "plugin:hframes:border_size_" + std::to_string(i + 1), Hyprlang::INT{-1});
    }

    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hframes:border_type", Hyprlang::STRING{"chamfer"});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hframes:border_tex", Hyprlang::STRING{""});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hframes:border_norm", Hyprlang::STRING{""});
  
    /*
    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprblinds-theme", onNewTheme, Hyprlang::SHandlerOptions{});
    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprblinds-tex", onAddTex, Hyprlang::SHandlerOptions{});
    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprblinds-norm", onAddNorm, Hyprlang::SHandlerOptions{});
    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprblinds-tile", onAddTile, Hyprlang::SHandlerOptions{});
    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprblinds-class", onSetClass, Hyprlang::SHandlerOptions{});
    */

    static auto P0 = HyprlandAPI::registerCallbackDynamic(PHANDLE, "preConfigReload", [&](void* self, SCallbackInfo& info, std::any data) { onPreConfigReload(); });
    static auto P1 = HyprlandAPI::registerCallbackDynamic(PHANDLE, "openWindow", [&](void* self, SCallbackInfo& info, std::any data) { onNewWindow(self, data); });

    g_pGlobalState = makeUnique<SHFGlobalState>();
    initGlobal();

    // add deco to existing windows
    for (auto& w : g_pCompositor->m_vWindows) {
        if (w->isHidden() || !w->m_bIsMapped)
            continue;

        HyprlandAPI::addWindowDecoration(PHANDLE, w, makeUnique<CHFrames>(w));
    }

    HyprlandAPI::reloadConfig();

    HyprlandAPI::addNotification(PHANDLE, "[hyprframes] Initialized successfully!", CHyprColor{0.2, 1.0, 0.2, 1.0}, 5000);

    return {"hyprframes", "A plugin to add chamfered/img borders to windows.", "Ren", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    wl_event_source_remove(g_pGlobalState->tick);
    g_pHyprRenderer->m_sRenderPass.removeAllOfType("CHFPassElement");
}
