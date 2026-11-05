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

# Validacion 0.9

Fecha: 1 de noviembre de 2026.

- Compilacion y enlace NRO para Horizon ARM64 con devkitA64 GCC 15.2.0.
- Parser M3U y codecs de la API: pruebas aprobadas.
- Transporte Ranger: 16 vectores ARM64 originales aprobados; limites, cargas
  binarias, codificacion invalida y relleno malformado rechazados correctamente.
- Integracion HTTPS local del login heredado, activacion y estado persistente,
  catalogo TV/VOD, series, episodios, busqueda y paginacion: aprobada.
- Integracion HTTPS local Ranger: TV y VOD, separacion de IV por ruta y direccion,
  vinculo de token/portal, licencia y autorizacion CDN, preservacion de URL firmada.
- Casos negativos Ranger: region rechazada, reproduccion rechazada, configuracion
  CDN rechazada, HTTP403 del CDN, HTTP302 sin seguir redireccion, cifrado invalido
  y URL de proxy loopback no implementado: aprobados. No se continua el flujo
  despues del rechazo y no se sustituye el video por el demo.
- Reproduccion HTTP local con Content-Auth y Content-License: dos ciclos completos
  con imagen no vacia, ventana/renderer persistentes y regreso al menu. El ensayo
  silencia el audio de PC. La prueba real de video/audio local en Switch fue
  confirmada por el usuario con la base 0.3.
- Ensayo real autorizado con una identidad de desarrollo separada: el servicio
  respondio portal100024 y se detuvo la activacion. El texto se comprobo en los
  recursos de la APK: Restricciones regionales, no se puede usar en esta area.
- No hay prueba satisfactoria de reproduccion real del proveedor con 0.8 ni de
  ejecucion de este NRO en una Switch durante esta sesion.

Los resultados locales no demuestran que el CDN real entregue URLs listas para
mpv. Firmas sign2, proxy, indices propios, P2P/CDP/WebRTC, medios cifrados especiales,
renovacion de licencias y DRM permanecen fuera del soporte implementado.

## Correcciones 0.8

- APK: VodCategoryActivity -> pd/u.Y3 -> GetShelveBean -> getShelveData v3;
  columnType 2, pageSize/numDisplay 24, encryptVersion 0. Respuesta assetList
  y assetListTotalSize. Prueba local de categoria -> titulos sin recursar categorias.
- ARM64: RequestSLB 0x3803b4 agrega Content-Type application/octet-stream.
  Constructor HTTP 0x5d5860 almacena enum 1; tabla 0x28c590 registra 1=GET, 2=POST.
  Se conserva GET sin cuerpo y sin cabeceras de sesion del portal.
- Dos autorizaciones Live sinteticas ejecutadas con el constructor original
  0x2ff160 coinciden con el transporte implementado (86 y 107 caracteres).
- El mock HTTPS rechaza con 400 si falta la cabecera nativa. Esta prueba
  comprueba el contrato observado en la APK, no la causa unica del error real.
- HTTP400 real y carga de titulos en la consola pendientes de validacion.

## Correcciones 0.9

- Reporte de consola de 0.8: solo Mas titulos en VOD y TV aun HTTP400.
- u8/g.java:78 usa AssetList.type en extra_type. programType va a estadisticas.
  Se corrigio parser tanto de categorias como de busqueda. Fixture con type 1/0
  y programType textual reproduce la diferencia que las pruebas anteriores omitieron.
- Native SetEnv 0x44854c, bloque 0x44d204-0x44d350: inserta Ranger-Id, App y
  App-Version en el mapa de cabeceras +0x320 que RequestSLB 0x3803b4 copia.
- ID 0x674b20: 24 bytes con checksum Murmur3 x86_32 seed42, usuario, Android ID,
  segundo de creacion y nonce; alfabeto rotado y prefijo de dos caracteres hex.
  Tres ejecuciones del ARM64 original con entradas sinteticas coinciden con C++;
  decoder Python independiente verifica checksum, campos y tiempo en el mock HTTPS.
- El ID usa el usuario de la sesion y Android ID cero porque bootstrap registra
  androidId vacio. Se usa nonce aleatorio nuevo, sin alterar el estado de activacion.
- HTTP/1.1 como el constructor nativo; conservadas verificaciones TLS y prohibicion
  de seguir redirecciones del resolvedor. Sin envio de userToken al CDN.
- Caso CDN400 con respuesta que refleja autorizacion: fallo visible, sin guardar
  esa respuesta ni sus secretos en diagnostico.log. Solo se guarda estructura.
- No se reintento activar otra identidad tras el rechazo regional de desarrollo.
- La solucion del HTTP400 y reproduccion real de 0.9 NO estan confirmadas.

## Entrega 0.10

- Reporte real del usuario: peliculas reproducen con 0.9; TV falla leyendo respuesta
  cifrada. El HTTP400 anterior ya no es el mensaje observado. No se declara TV resuelta.
- Original ARM64 0x6790ac ejecutado con shim memchr correcto: 16 bytes sinteticos
  decodifican igual con Base64 sin relleno, con == y con == seguido de CRLF.
  La entrada sin relleno seguida de LF y la entrada entre comillas se rechazan.
  Se admite el relleno canonico comprobando AES/PKCS7; no se debilita el padding.
- HTTPS local: respuestas con relleno, gzip y HTML inesperado, ademas de todos
  los casos anteriores. Se conserva rechazo de autorizacion y redirecciones.
- VOD: seleccion de la segunda calidad 1080p/English, solicitud media_code correcto,
  descarga de subtitulo sin cabeceras de sesion/medio y rechazo por MD5 incorrecto.
- libmpv real, 8 escenarios sobre video sintetico con 2 pistas de audio y SRT:
  pausa/continuacion, seek exacto, aid=2, sid=1, velocidad=1.25, proporcion 16:9,
  seleccion de calidad y episodio, SRT externo sid=2, pausa activa, menu,
  siguiente automatico desactivado, volumen=95, audio-delay=0.1, sub-delay=0.25,
  sub-scale=1.1. Ventana/renderer conservados. Audio de PC silenciado.
- Progreso/preferencias: guardar/restaurar, finalizacion, reinicio, JSON malformado,
  limites de volumen/velocidad/posicion y maximo 500 entradas verificados.
- Captura interna del menu revisada visualmente a 1280x720; filas y barra legibles.
- 0.10 no se ha ejecutado en Switch en esta sesion. Audio multidioma, avance de
  series, nuevas calidades y TV del proveedor requieren prueba real del usuario.
- El selector de motores Android, feedback, interfaz exacta, render de mayor
  resolucion, variantes HLS manuales y los transportes Ranger documentados siguen
  pendientes. No se presenta esta entrega como equivalencia completa con la APK.

## Entrega 0.11: peticion al video de TV

- Usuario: las demas funciones de 0.10 ya funcionan; TV abre reproductor y falla
  con loading failed. No se reintento activar otra identidad de desarrollo.
- Native BuildReq 0x46de48 / bloque 0x46ed08: Uri::resource() 0x63b534 obtiene
  path, query y fragment. Firmante 0x6752e0 genera la cabecera Content-Auth.
  La URL y Content-License se conservan; la autorizacion SLB no es Content-Auth.
- Firmante ARM64 original ejecutado sobre cuatro entradas sinteticas: token
  normal, token con escapes, instancia negativa y recurso sin token. Los hashes
  coinciden con C++. Se corrigio el shim snprintf %ld para signed int64.
- Firma sign_o3: modified MD5 previamente validado, bytes exactos del protocolo,
  instancia 0 y momento en milisegundos. Sin token utilizable devuelve el recurso.
- Cabeceras de video: Content-Auth, Content-License, App, App-Version, Ranger-Id,
  Pragma=akamai-x-cache-on. La allowlist de mpv rechaza campos ajenos y controles.
- HTTPS API local: TV sin firma y con firma, URL intacta, VOD y segunda calidad,
  SRT/MD5 y todos los rechazos anteriores. Python verifica la firma independiente.
- libmpv real / HTTP local: peticion antigua rechazada con 403; error clasificado
  sin escribir mensajes crudos ni secretos. Peticion con cabeceras/firma validas
  reproduce imagen y vuelve al mismo menu dos veces. La prueba anterior de
  cabeceras simples tambien reproduce dos veces. Sonido de PC silenciado.
- Etiquetas de error: pruebas de HTTP403/404/503, TLS y formato; textos con URL
  o cabecera privada sin patron de error se descartan. El evento de libmpv usa
  nivel v porque FFmpeg emite algunos estados HTTP ahi; solo salen etiquetas fijas.
- No se confirma TV real en consola con 0.11. La firma por cada fragmento HLS,
  cambio de URI en redirects, renovacion de licencias y proxy nativo no estan completos.

## Entrega 0.12: ruta del canal de TV

- Log real adjunto de 0.11: bootstrap aceptado, SLB HTTP200, Live auth firmado,
  luego mpv=-13 y HTTP404 en cuatro intentos; sin primer frame. La causa HTTP
  esta probada por el log. La correccion de ruta necesita confirmacion en consola.
- Reconstruccion 0x47db34: copia la URI de media_url y sobreescribe solo su path.
  En SLB, 0x480314 elige .mp4 para AVFormat=mp4 y .ts para los demas formatos.
  0x4804f4 concatena /live/ + estado Media.name (offset 0x390) + extension.
  Java h8/v3.E crea Media.name desde LiveAddress.playCode y format desde AVFormat.
- Unicorn ejecuta las instrucciones originales, sin parches, de ambos bloques:
  codigo channel-001/formato vacio, channel_ABC/ts, test-channel/mpegts y
  channel-mp4/mp4. Fixture independiente tests/fixtures/live_path_arm64_vectors.json.
  El shim compareEmmPKcm reproduce la comparacion de substring de libc++.
- Se conserva autoridad, consulta y fragmento; se sustituye unicamente el path.
  Content-Auth sigue derivandose de la URI CDN original, no del path resultante.
- HTTPS local verifica rutas TS y MP4 y firma separada con el path original.
  VOD y todos los rechazos/controles de autorizacion anteriores pasan.
- Reproductor: se toma el diagnostico clasificado tambien al final, despues de
  vaciar la cola. El log real mostro el HTTP404 posterior al evento END_FILE.
- Ensayo libmpv/HTTP incluye ruta antigua con 404 y ruta corregida, MPEG-TS
  H.264/AAC y MP4 sinteticos, cabeceras obligatorias y dos ciclos por medio.
- Sin nueva activacion ni acceso remoto al proveedor. TV real aun no confirmada.
