# Xuper TV en Windows

Port de escritorio del cliente: mismo codigo C++17, SDL2, libmpv y cURL; solo
cambia la capa de plataforma (`include/platform.hpp`, `include/gamepad.hpp`) y el
sistema de build (`CMakeLists.txt`). La compilacion para Switch no cambia.

## Que incluye y que no

- Es el **cliente**. Este repositorio no contiene servidores, catalogos ni
  cuentas, ni en Switch ni en Windows. La app lee el mismo `service.json` que
  usas en la consola; copialo junto a `XuperTV.exe` y todo se comporta igual
  (mismo flujo de arranque, catalogo, favoritos, continuar viendo y reproductor).
- No se incluye ni se genera configuracion de ningun servicio de terceros.

## Compilar (MSYS2, recomendado)

1. Instala [MSYS2](https://www.msys2.org) y abre la terminal **MSYS2 MINGW64**.
2. Dependencias:

   ```
   pacman -S --needed git mingw-w64-x86_64-{gcc,cmake,ninja,pkgconf,SDL2,SDL2_ttf,mpv,curl,libpng,libjpeg-turbo,libwebp}
   ```

3. Compilar y probar (mbedTLS 3.6 se descarga y compila solo):

   ```
   cmake -S . -B build-win -G Ninja -DXUPER_FETCH_MBEDTLS=ON
   cmake --build build-win
   ctest --test-dir build-win --output-on-failure
   ```

4. Carpeta portable con las DLL: `bash packaging/windows/package.sh build-win dist/Xuper-TV-Windows`

`.github/workflows/build-windows.yml` hace lo mismo en GitHub Actions y publica
la carpeta como artefacto. En Linux tambien compila con el mismo `CMakeLists.txt`.

## Archivos junto al .exe

El programa trabaja siempre en la carpeta del `.exe`: `service.json`,
`device_state.json`, `library_state.json`, `playback_state.json`, `catalog.m3u`,
`artwork/` y `diagnostico.log`. Son los mismos nombres que en `sdmc:/switch/xuperswitch/`.

- `font.ttf` es opcional: si falta usa Segoe UI/Arial de Windows.
- `cacert.pem` es opcional: si falta, cURL usa el almacen de certificados de Windows.

## Controles

| Accion | Teclado | Mando (Xbox/DualSense via SDL) |
| --- | --- | --- |
| Moverse | Flechas | D-pad o palanca izquierda |
| Elegir / reproducir | Enter | A |
| Volver | B | B |
| Buscar | X | X |
| Activar / reintentar | Y | Y |
| Pagina anterior/siguiente | RePag / AvPag | LB / RB |
| Favorito | F | R3 |
| Apoyar el desarrollo | D | Back |
| Recargar lista local | L | LT |
| Salir | Esc | Start |

En el reproductor: Espacio/Enter o A elige o pausa; X, Tab o Start abre el menu;
Y cambia audio; S subtitulos; `[` `]` velocidad; F pantalla; M silencio;
J/L o LT/RT desplazan 60 s; RePag/AvPag o LB/RB cambian episodio.

## Cambios respecto a la version de Switch

- `mbedtls_md5_ret` (retirado en mbedTLS 3.x) usa ahora `md5_digest()`, valido para 2.x y 3.x.
- `std::aligned_alloc` no existe en Windows: se usa `_aligned_malloc/_aligned_free`.
- `mkdir(ruta, modo)` de POSIX se sustituye por `xs::make_dir`.
- Rutas relativas a la carpeta del `.exe` en lugar del directorio actual.
- Certificados nativos de Windows con cURL, y mando con la misma disposicion de botones que Switch.

## Estado de la verificacion

Verificado en Linux con el mismo codigo y CMake: compila con `-Wall -Wextra`, 8/8
pruebas con mbedTLS 2.28 y con 3.6, y la app arranca (SDL en modo sin pantalla).
**No se ha ejecutado en una maquina Windows real**: usa el workflow de GitHub Actions
o los pasos de arriba y reporta cualquier fallo de enlace o de libmpv.

Antes de distribuir un binario revisa las licencias de libmpv/FFmpeg de la variante
exacta que enlaces (ver `DEPENDENCIAS.md` y `COPYRIGHT.mpv`).
