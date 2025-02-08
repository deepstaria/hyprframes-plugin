{
  lib,
  hyprland,
  hyprlandPlugins,
}:
hyprlandPlugins.mkHyprlandPlugin hyprland {
  pluginName = "hyprframes";
  version = "0.1";
  src = ./.;

  inherit (hyprland) nativeBuildInputs;

  meta = with lib; {
    homepage = "https://github.com/deepstaria/hyprframes-plugin";
    description = "Hyprframes plugin";
    license = licenses.bsd3;
    platforms = platforms.linux;
  };
}
