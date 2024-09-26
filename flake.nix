{
  description = "Dockerized android emulator";

  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = {...} @ inputs:
    inputs.flake-utils.lib.eachDefaultSystem (system: let
      pkgs = (import inputs.nixpkgs) {
        inherit system;
        config = {
          allowUnfree = true;
        };
      };
    in {
      packages = rec {
        default = lockdev-redirect;
        lockdev-redirect-so = pkgs.stdenv.mkDerivation {
          name = "lockdev-redirect";
          src = ./.;
          installPhase = ''
            mkdir -p $out/lib
            cp lockdev-redirect.so $out/lib
          '';
        };
        lockdev-redirect = pkgs.writeShellScriptBin "lockdev-redirect" ''
          if [ "$1" = '--version' ]; then
            echo 'lockdev-redirect version 1.0.0'
            exit 0
          fi

          if [ "$1" = '--help' ]; then
            echo 'lockdev-redirect, redirect /var/lock to a user-writable path.'
            echo 'Usage: lockdev-redirect COMMAND'
            echo 'Where COMMAND is the command to execute with redirected /var/lock'
            exit 0
          fi

          export LD_PRELOAD=$LD_PRELOAD:${lockdev-redirect-so}/lib/lockdev-redirect.so
          exec $*
        '';
      };
      # devShells.default = pkgs.mkShell {
      #   packages = [
      #   ];
      # };
    });
}
