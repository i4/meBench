# SPDX-License-Identifier: GPL-2.0-or-later
{
  description = "Synthetic Benchmarking Tool for energy-related measurements";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
    flake-parts.url = "github:hercules-ci/flake-parts";
    flake-parts.inputs.nixpkgs-lib.follows = "nixpkgs";
    treefmt-nix.url = "github:numtide/treefmt-nix";
    treefmt-nix.inputs.nixpkgs.follows = "nixpkgs";
    git-hooks-nix.url = "github:cachix/git-hooks.nix";
    git-hooks-nix.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = {
    self,
    nixpkgs,
    flake-parts,
    ...
  } @ inputs:
    flake-parts.lib.mkFlake {inherit inputs;} {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      imports = [
        inputs.git-hooks-nix.flakeModule
        inputs.treefmt-nix.flakeModule
      ];

      perSystem = {
        config,
        self',
        inputs',
        pkgs,
        system,
        ...
      }: let
        pkgs = import nixpkgs {inherit system;};
      in {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            clang
            gnumake
            man-pages
            man-pages-posix
            valgrind
            gdb
            numactl.dev
            pkg-config
            python312
          ];
          shellHook = ''
            ${config.pre-commit.installationScript}
          '';
        };

        pre-commit.settings.hooks = {
          treefmt.enable = true;
        };

        treefmt = {
          programs = {
            alejandra.enable = true;
            clang-format.enable = true;
          };
          settings.global.excludes = [
            ".envrc"
            "README.md"
          ];
        };
      };
    };
}
