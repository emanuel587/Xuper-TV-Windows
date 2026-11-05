## Entrega 0.14 — interfaz y Favoritos

- Reporte del usuario: TV, peliculas y series funcionan con la base anterior.
- Arranque automatico, marca by Jona Ruacho, menu lateral, cuadrillas,
  metadatos PosterList.fileUrl de los modelos originales y fichas VOD.
- Busqueda de canales por categorias accesibles, normalizacion de acentos,
  cache RAM con limite de 12000 canales/64 categorias; VOD conserva endpoint.
- Favoritos persistentes (R3/tactil), recents deduplicados, recuperacion .bak,
  rollback ante fallo de escritura. No se persisten URL de video ni cuentas.
- Descarga de arte en un trabajador: sin cabeceras privadas, TLS verificado,
  sin downgrade HTTPS en redirects; limite 3 MiB/respuesta y dimensiones;
  decodificacion PNG/JPEG/WebP. Cache SD limitada, texturas maximo 18.
- UI probada con SDL real y catalogos sinteticos, imagenes verificadas,
  prueba de arranque sin Y + busqueda teclado + favorito + navegacion tactil
  + donacion + cierre. Prueba de cache entre dos ejecuciones aprobada.
- QR integrado: URL exacta del usuario, lectura independiente aprobada.
  No se verifico cuenta receptora ni se hizo ningun pago.
- Regresion de reproduccion: dos ciclos MP4 base, HTTP403 y ruta404 clasificados,
  dos ciclos TV TS y dos TV MP4 con firma/cabeceras, regreso a renderer persistente.
- Compilacion NRO0/ARM64 0.14.0 aprobada. No probado en Switch en esta sesion.
- No se agrega bloqueo de copia ni cambia la licencia del codigo ya distribuido.

La interfaz nueva agrega enlace con libpng16, libjpeg y libwebp de portlibs,
ademas de los componentes previos. No se agrega SDL2_image al ejecutable.
Los encabezados/bibliotecas del host deben coincidir con su ABI (JPEG80 en
el host de validacion, JPEG62 en Switch); no mezclar esos encabezados.

## Actualizacion 0.13

- Usuario: algunos canales funcionan; registro 0.12 con siete primeros frames.
- Resolvedor HTTP explicito aceptado; sin esquema conserva HTTPS. HTTPS se
  verifica; no se siguen redirecciones HTTP/HTTPS del resolvedor.
- Prueba local de 19 casos Ranger, incluidos HTTP correcto, HTTP302 detenido,
  HTTPS302 a HTTP detenido y esquema FTP rechazado antes de enviar nada.
- Activacion HTTP y subtitulo HTTP rechazados antes de enviar solicitudes.
  Los POST de cuenta solo llegan al servidor TLS local; no van al HTTP.
- Historial: escritura temporal, copia anterior, instalacion en destino libre,
  recuperacion con primario ausente o incompleto, fallo de escritura preserva
  archivo anterior. Los errores registran etapas fijas, sin datos privados.
- Regresion: vectores originales ARM64 de transporte/ID/firma/ruta y errores
  privados de reproduccion aprobados. NRO 0.13.0 compilado para Horizon ARM64.
- No se probo TV real 0.13 en este entorno; requiere validacion en Switch.

# Compilacion de XuperSwitch Lab 0.9

## Resultado de esta entrega

Se compilo y enlazo `xuperswitch.nro` para Horizon ARM64 con devkitA64 GCC 15.2.0.
Se comprobo la cabecera NRO0 y la arquitectura AArch64 del ELF de enlace. La
compilacion no equivale a una ejecucion satisfactoria en la consola.

## Herramientas empleadas

Se obtuvieron las herramientas y portlibs de la imagen oficial
`devkitpro/devkita64`. Ademas se instalaron los siguientes paquetes publicados por
el mantenedor de wiliwili, que incluyen la variante con render API necesaria:

- `switch-ffmpeg-7.1-1-any.pkg.tar.zst`
  SHA-256: `993e8ce950cf43040f1c213daf66c6dd4cd7e89d731ea6239856a76b40d65fa6`
- `switch-libmpv-0.36.0-3-any.pkg.tar.zst`
  SHA-256: `672aa214a6cb811db7c39bdeebe4462e3a2eed47c53eac8b562b7ccd8fc3c859`

Descarga: https://github.com/xfangfang/wiliwili/releases/tag/v0.1.0

No se usa la variante `deko3d` de libmpv en este prototipo.

## Windows con devkitPro MSYS2

Instala devkitPro con soporte Switch. En su terminal instala/actualiza las
portlibs de SDL2, SDL2_ttf, Mesa, cURL y las dependencias de los paquetes de medios. Instala
los dos paquetes anteriores con `pacman -U RUTA_AL_PAQUETE`. El gestor debe
resolver las dependencias declaradas. No utilices bibliotecas de PC para enlazar.

Desde la carpeta del proyecto:

```sh
make
```

El Makefile busca los paquetes `mpv`, `sdl2`, `SDL2_ttf` y `libcurl` mediante el wrapper
`aarch64-none-elf-pkg-config`. Sus dependencias incluyen libnx, mbedTLS, FFmpeg,
libass, HarfBuzz, FreeType, fribidi, dav1d, Lua 5.1 y Mesa.

## Pruebas de componentes en PC

Se necesitan g++, make, cabeceras/bibliotecas de mbedTLS y cURL, Python y
cryptography para las pruebas TLS. La cabecera JSON esta incluida.

```sh
make -f Makefile.host test
make -f Makefile.host test-service
```

Se pueden proporcionar `MBEDTLS_CFLAGS`, `MBEDTLS_LIBS`, `CURL_CFLAGS` y
`CURL_LIBS` cuando las dependencias no esten en sus rutas de sistema. Las pruebas
de integracion utilizan exclusivamente un servidor HTTPS en localhost con una
CA temporal; no prueban la cuenta ni los servidores reales de XuperTV.

## Regenerar la configuracion desde la misma APK

```sh
python -m pip install -r tools/requirements.txt
python tools/configure_from_apk.py RUTA_A_LA_APK --output service.json
```

El extractor no ejecuta la APK ni hace solicitudes de red. Esta adaptado a las
clases y recursos de la muestra analizada; rechaza APKs que no coincidan.

## Fuentes externas

- https://devkitpro.org/wiki/Getting_Started
- https://github.com/switchbrew/libnx
- https://github.com/xfangfang/wiliwili
- https://github.com/averne/mpv
- https://github.com/nlohmann/json/tree/v3.12.0


## Pantalla y fuentes en 0.3

La interfaz usa SDL2_ttf con DejaVuSans.ttf, incluida como font.ttf y con su aviso
de licencia. El render de libmpv es software a 640x360; SDL conserva una unica
ventana y renderer para el menu y el video. No se inicia una consola libnx ni
se destruye la ventana al regresar de un video.

El ensayo tests/playback_smoke.cpp se compila en PC con XUPERSWITCH_TEST, SDL2,
SDL2_ttf y libmpv. Esa opcion silencia la salida de audio y admite rutas locales
absolutas solo en el binario de ensayo, nunca en el NRO. Ejecutarlo con
SDL_VIDEODRIVER=dummy permite comprobar dos reproducciones sin GPU fisica.


## Flujo automatico 0.4

Y llama a service_bootstrap. La configuracion extrae o7/a.i para apk_version y
conserva manifest_version por separado. installation_serial es un valor aleatorio
persistente de esta instalacion. device_state.json se crea en la SD y conserva
solo sn y open_count; no se incluye al distribuir el ZIP.

## Catalogo 0.5

La sesion obtenida por active se conserva en RAM. Y consulta categorias; A carga
canales y solicita startPlayLive antes de reproducir. Las pruebas HTTPS locales
incluyen fuente directa, fuente opaca, licencia, rechazo y entradas restringidas.
El reproductor mantiene la misma implementacion de 0.3.

## Navegacion VOD y busqueda 0.6

El teclado Switch usa swkbd de libnx. X busca y ZL abre la lista local. Se agregan
pruebas locales de filterByContent, getItemData, startPlayVOD y searchByName,
incluidas paginas sucesivas y rechazo al solicitar un episodio. RANGER.md explica
por que la reproduccion propietaria sigue pendiente.

## Ranger HTTP 0.9

El enlace nativo incluye ranger_codec.o. Pruebas de componentes y red local:

```sh
make -f Makefile.host test test-ranger test-service
```

La prueba opcional tests/test_playback_http.py ejecuta playback_smoke.cpp
compilado con XUPERSWITCH_TEST. Recibe binario, test.mp4 y font.ttf como argumentos;
usa HTTP local con cabeceras obligatorias, renderer SDL dummy y audio silenciado.

La prueba del proveedor real se detuvo por rechazo regional. No se considera
una verificacion satisfactoria del servicio o del NRO en hardware.

Las pruebas Ranger incluyen 3 vectores originales de Ranger-Id y validacion
independiente de cabeceras HTTP/1.1. VOD usa type, no programType.

0.10 agrega test-state y tests/playback_controls.cpp. El ensayo de controles
requiere SDL2/SDL_ttf/libmpv host, un video con dos audios y SRT interno, y
XUPERSWITCH_TEST para silenciar salida de audio. No confundir con prueba de consola.

0.11 agrega test-errors y 4 vectores Live Content-Auth. El playback_smoke host
ahora requiere source/ranger_codec.cpp y mbedcrypto. test_playback_http.py valida
headers, rechazo HTTP403 diagnosticado y peticion Live firmada; todo en localhost.

0.12 agrega 4 vectores ARM64 de Live path. test_playback_http.py acepta un cuarto
argumento opcional de video MPEG-TS. Se puede convertir el demo H264/AAC:
ffmpeg -i test.mp4 -c copy -bsf:v h264_mp4toannexb -f mpegts test.ts
La prueba mantiene la URI de autorizacion distinta de la ruta del medio.
