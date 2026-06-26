{
  lib,
  stdenv,
  cmake,
  pkg-config,
  aquamarine,
  cairo,
  hyprgraphics,
  hyprlang,
  hyprtoolkit,
  hyprutils,
  libdrm,
  libGL,
  pango,
  pixman,
  libxkbcommon,
  version ? "0",
}:
let
  inherit (lib.sources) cleanSource cleanSourceWith;
  inherit (lib.strings) hasSuffix;
in
stdenv.mkDerivation {
  pname = "hyprland-guiutils";
  inherit version;

  src = cleanSourceWith {
    filter =
      name: _type:
      let
        baseName = baseNameOf (toString name);
      in
      !(hasSuffix ".nix" baseName);
    src = cleanSource ../.;
  };

  nativeBuildInputs = [
    cmake
    pkg-config
  ];

  buildInputs = [
    aquamarine
    cairo
    hyprgraphics
    hyprlang
    hyprtoolkit
    hyprutils
    libdrm
    libGL
    libxkbcommon
    pango
    pixman
  ];

  meta = {
    description = "Hyprland GUI utilities (successor to hyprland-qtutils)";
    homepage = "https://github.com/hyprwm/hyprland-guiutils";
    license = lib.licenses.bsd3;
    maintainers = [ lib.maintainers.fufexan ];
    platforms = lib.platforms.linux;
  };
}
