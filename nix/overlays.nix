{
  inputs,
  self,
  lib,
}:
let
  mkDate =
    longDate:
    (lib.concatStringsSep "-" [
      (builtins.substring 0 4 longDate)
      (builtins.substring 4 2 longDate)
      (builtins.substring 6 2 longDate)
    ]);
  date = mkDate (self.lastModifiedDate or "19700101");
  version = lib.removeSuffix "\n" (builtins.readFile ../VERSION);
in
{
  default = self.overlays.hyprland-guiutils;

  hyprland-guiutils-with-deps = lib.composeManyExtensions [
    inputs.aquamarine.overlays.default
    inputs.hyprgraphics.overlays.default
    inputs.hyprlang.overlays.default
    inputs.hyprtoolkit.overlays.default
    inputs.hyprutils.overlays.default
    self.overlays.hyprland-guiutils
  ];

  hyprland-guiutils = final: prev: {
    hyprland-guiutils = final.callPackage ./. {
      stdenv = final.gcc15Stdenv;
      version = "${version}+date=${date}_${self.shortRev or "dirty"}";
    };
  };
}
