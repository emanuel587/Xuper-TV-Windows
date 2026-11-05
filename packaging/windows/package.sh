
#!/usr/bin/env bash
# Arma una carpeta portable para Windows: XuperTV.exe + DLL + archivos de servicio.
# Ejecutar despues de compilar:
# packaging/windows/package.sh build-win dist/Xuper-TV-Windows

set -euo pipefail

build=${1:?Falta la carpeta de build (por ejemplo build-win)}
out=${2:?Falta la carpeta de salida (por ejemplo dist/Xuper-TV-Windows)}

exe="$build/XuperTV.exe"

[ -f "$exe" ] || {
    echo "No existe $exe; compila primero." >&2
    exit 1
}

mkdir -p "$out"

# Copiar ejecutable
cp "$exe" "$out/"

# Copiar DLL necesarias
ldd "$exe" |
    awk '/=>/ {print $3}' |
    grep -iE '/(mingw64|ucrt64|clang64|mingw32)/' |
    sort -u |
    while read -r dll; do
        cp -u "$dll" "$out/"
    done

# Copiar archivos de configuración y licencias
cp service.example.json LICENSE LICENSE.nlohmann-json COPYRIGHT.mpv DEPENDENCIAS.md WINDOWS.md "$out/"

# Copiar configuración y certificado para Windows
cp packaging/windows/service.json packaging/windows/cacert.pem "$out/"

# Copiar licencias
cp -r licenses "$out/"

# Copiar instrucciones de Windows
cp packaging/windows/LEEME_WINDOWS.txt "$out/LEEME_PRIMERO.txt"

echo "Listo: $out"
echo "Contenido del paquete:"
ls "$out"
