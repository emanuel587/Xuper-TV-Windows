#!/usr/bin/env bash
# Arma una carpeta portable para Windows: XuperTV.exe + las DLL que necesita.
# Ejecutar dentro de una terminal MSYS2 MINGW64, despues de compilar:
#   packaging/windows/package.sh <carpeta-de-build> <carpeta-de-salida>
set -euo pipefail
build=${1:?Falta la carpeta de build (por ejemplo build-win)}
out=${2:?Falta la carpeta de salida (por ejemplo dist/Xuper-TV-Windows)}
exe="$build/XuperTV.exe"
[ -f "$exe" ] || { echo "No existe $exe; compila primero." >&2; exit 1; }

mkdir -p "$out"
cp "$exe" "$out/"

# Copia toda DLL no perteneciente a Windows que el programa necesite (ldd es transitivo).
ldd "$exe" | awk '/=>/ {print $3}' | grep -iE '/(mingw64|ucrt64|clang64|mingw32)/' | sort -u |
while read -r dll; do cp -u "$dll" "$out/"; done

# Avisos de licencia que exige el proyecto, y la plantilla de campos (valores ficticios).
cp service.example.json LICENSE LICENSE.nlohmann-json COPYRIGHT.mpv DEPENDENCIAS.md WINDOWS.md "$out/"
cp -r licenses "$out/"
cp packaging/windows/LEEME_WINDOWS.txt "$out/LEEME_PRIMERO.txt"

echo "Listo: $out"
ls "$out"
