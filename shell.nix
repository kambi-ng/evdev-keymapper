{ pkgs ? import <nixpkgs> {} }:

with pkgs;

mkShell {
  buildInputs = [
    pkgs.linuxHeaders pkgs.fd pkgs.bear
  ];
}
