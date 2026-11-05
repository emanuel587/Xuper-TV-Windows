# Dependencias y procedencia

El codigo original del cliente se encuentra completo en source/, include/ y
las pruebas. El JSON de nlohmann 3.12.0 se incluye con su licencia MIT.

El NRO enlaza bibliotecas libres para Switch. Los proyectos originales y sus
fuentes, licencias y recetas de compilacion estan disponibles en:

- libnx y herramientas: https://github.com/switchbrew/libnx y
  https://github.com/devkitPro/pacman-packages
- SDL2 para Switch: https://github.com/devkitPro/SDL
- Mesa y portlibs: https://github.com/devkitPro/mesa y
  https://github.com/devkitPro/pacman-packages
- libmpv para Switch: https://github.com/averne/mpv y las recetas de medios de
  https://github.com/xfangfang/wiliwili (paquete 0.36.0-3, variante OpenGL).
- FFmpeg para Switch: https://github.com/averne/FFmpeg y las recetas de medios de
  https://github.com/xfangfang/wiliwili (paquete 7.1-1).
- cURL: https://github.com/curl/curl
- mbedTLS: https://github.com/Mbed-TLS/mbedtls

Los paquetes empleados y sus huellas estan en BUILD.md. La imagen oficial de
herramientas utilizada es devkitpro/devkita64. No se modificaron las bibliotecas
precompiladas. Sus licencias son independientes de la licencia MIT del codigo
original y se conservan al redistribuir o volver a compilar el programa.

La APK y sus bibliotecas propietarias no se distribuyen en este paquete. El
extractor obtiene parametros de configuracion de la copia proporcionada por el
usuario; no incluye claves de cuentas ni permite omitir permisos del servidor.


Interfaz 0.3: SDL2_ttf (https://github.com/libsdl-org/SDL_ttf) y DejaVu Sans.
El aviso de la fuente se incluye en licenses/DejaVu-font.txt. El programa de
prueba usa una pantalla virtual SDL en PC; esas bibliotecas de PC no se
incorporan al ejecutable para Switch.

Interfaz 0.14: PNG con libpng, JPEG con libjpeg-turbo y WebP con libwebp,
ya disponibles en portlibs. Se conservan sus avisos de licencia. El QR se
genera en desarrollo con ReportLab; el ejecutable solo incluye su matriz.

This software is based in part on the work of the Independent JPEG Group.
