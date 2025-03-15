{
  description = "evdev keymapper service";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }: let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
    lib = pkgs.lib;
    tomlHeader = pkgs.fetchurl {
    url = "https://raw.githubusercontent.com/marzer/tomlplusplus/30172438cee64926dc41fdd9c11fb3ba5b2ba9de/toml.hpp";
    sha256 = "0gkq35449zid4gx2lnqy2cs252m3lwgih6drczn9llfn9nnp4lbb"; # Run `nix-prefetch-url <url>` to get the hash
    };

    # 1. Define the package (Builds the service)
    evdev-keymapper-package = pkgs.stdenv.mkDerivation {
      pname = "evdev-keymapper";
      version = "0.3.0";

      src = ./.;

      nativeBuildInputs = [  pkgs.makeWrapper ];
      buildInputs = [ ]; # Modify based on your project

      buildPhase = ''
        mkdir -p vendor
        cp ${tomlHeader} vendor/toml.hpp
        make
      '';

      installPhase = ''
        install -Dm755 evdev-keymapper $out/bin/evdev-keymapper
      '';

      meta = {
        description = "evdev-keymapper";
        license = lib.licenses.mit;
        platforms = lib.platforms.linux;
      };
    };

    evdev-keymapper-service = { config, lib, pkgs, ... }: let
      cfg = config.services.evdev-keymapper;
      tomlFormat = pkgs.formats.toml { };
      tomlFile = tomlFormat.generate "config.toml" cfg.settings;
    in {
      options.services.evdev-keymapper = {
        enable = lib.mkEnableOption "Enable evdev-keymapper";

        package = lib.mkOption {
          type = lib.types.package;
          default = evdev-keymapper-package;
          description = "The package providing the service binary.";
        };

        settings = lib.mkOption {
          type = lib.types.attrs;
          default = {
            Config = {
              toggle=false;
              device="/dev/input/event0";
            };
            Keymap = {
              "RIGHTALT"="CAPSLOCK";
              CAPSLOCK = {
                "I"="UP";
                "K"="DOWN";
                "J"="LEFT";
                "L"="RIGHT";
                "B"="BACKSPACE";
                "F"="ESC";
                "N"="LEFTSHIFT+MINUS";
                "APOSTROPHE"="GRAVE";
              };
            };
          };
          description = "Configuration for the evdev-keymapper.";
        };
      };

      config = lib.mkIf cfg.enable {
        systemd.services.evdev-keymapper = {
          description = "evdev-keymapper: remap keys with evdev";
          wantedBy = [ "multi-user.target" ];
          serviceConfig = {
            ExecStart = "${cfg.package}/bin/evdev-keymapper ${tomlFile}";
            Restart = "always";

            # Enable strict sandboxing
            ProtectSystem = "strict";
            ProtectHome = true;
            NoNewPrivileges = true;
            PrivateTmp = true;
            ReadOnlyPaths = [ "${tomlFile}" ];
          };
        };

        services.udev.extraRules = ''
            KERNEL=="event*", ATTRS{phys}=="kambi-ng-udev", SYMLINK+="input/by-id/evdev-keymap"
        '';
      };
    };

  in {
    # Automatically add the package to pkgs
    overlays.default = final: prev: {
      evdev-keymapper = evdev-keymapper-package;
    };
    # Automatically make the service available
    nixosModules.default = evdev-keymapper-service;

    devShells.${system}.default = pkgs.mkShell {
      packages = [
        pkgs.gnumake
        pkgs.fd
        pkgs.linuxHeaders
      ];
      shellHook = ''
        export IN_NIX_SHELL=1
        echo "Dev shell ready"
      '';
    };
  };
}
