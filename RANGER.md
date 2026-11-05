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

# Ranger HTTP integrado en 0.8: soporte parcial

service_source ahora conecta el modulo de transporte con getSlbInfo y los
endpoints /slb/v9/live y /slb/v11/vod, utilizando la sesion obtenida mediante
activacion. El flujo respeta las licencias y sus etiquetas; la ruta VOD vincula
app_id y tag de la autorizacion con la licencia del video seleccionado.

TV usa la autorizacion completa de url_list como texto de la solicitud SLB.
VOD usa una lista JSON con auth, license y media_code. La respuesta se decodifica
con el IV resp de la misma variante y se selecciona una media_url HTTP(S).
Content-Auth y Content-License se entregan a mpv como un array de cabeceras para
TV. No se exportan userId/userToken a la solicitud CDN ni a los registros.

El resolvedor no sigue redirecciones con autorizacion, no reproduce URLs de
loopback que dependan del proxy Android y no acepta un rechazo como permiso.
El control de rechazo regional muestra portal100024 con su significado en la APK.

El usuario autorizo la prueba externa. Esta se realizo con una instalacion de
prueba separada y el servicio rechazo su activacion por region. La sesion de la
Switch no se copio al entorno de desarrollo. No se obtuvieron transmisiones
reales ni se continuaron consultas tras ese rechazo.

La integracion y cabeceras pasan pruebas HTTPS/HTTP locales. Esto no equivale a
portar todas las funciones nativas. Firmas sign2, proxy, formatos e indices
propietarios, P2P/CDP/WebRTC, renovaciones y DRM siguen pendientes. El usuario
puede probar la actualizacion con el estado de su Switch que ya activa; debe
conservar device_state.json.

Los apartados siguientes son evidencia y estados historicos del analisis.

---

# Avance de Ranger: transporte verificado, integracion pendiente

Se agregaron include/ranger_codec.hpp, source/ranger_codec.cpp y el objetivo
host test-ranger. El modulo realiza solamente transformaciones de datos, sin
conexion de red, seleccion de CDN, resolucion de medios ni reproduccion.
No se ha integrado a service_source ni al NRO 0.6 instalado por el usuario.

## Evidencia nueva de la misma APK

Se emularon rutinas ARM64 originales con Unicorn, sin conectar la biblioteca a
Internet. Los enlaces externos de libc/C++ se sustituyeron por operaciones
locales de memoria/cadenas. Se recuperaron 1138 bloques de inicializacion de
cadenas sin fallos de emulacion. El analisis de funciones se ayudo con Capstone
y una copia temporal con saltos indirectos simplificados; los vectores de prueba
se generaron ejecutando instrucciones ARM64 originales, no esa copia simplificada.

- 0x529178: variante de MD5. Cambia el orden de palabras de la primera ronda y
  cuatro constantes de rondas posteriores. No es intercambiable con MD5 normal.
- 0x2fee80 y 0x30121c: derivaciones para las variantes live/vod.
- 0x2fe9c0: IV a partir de una cadena de recurso mediante MurmurHash3 x64 de
  128 bits, semilla 1597135542. encrypt.io es parte de esa cadena; no se contacta.
- 0x678b14 y 0x678cf0: AES-CBC-128 con relleno PKCS#7. El modulo nuevo comprueba
  todos los bytes de relleno al decodificar.
- 0x678f48: base64 con alfabeto personalizado, sin signos de relleno al final.
- Recursos separados req:/slb/v9/live, resp:/slb/v9/live, req:/slb/v11/vod y
  resp:/slb/v11/vod. Las solicitudes y respuestas utilizan IV diferentes.

Los 16 vectores sinteticos en tests/fixtures/ranger_arm64_vectors.json coinciden
con las rutinas originales en derivacion, IV, cifrado y codificacion. Se prueban
ademas limites, cargas binarias, entradas malformadas y relleno invalido. Se
comprobo compilacion de esta unidad para Horizon ARM64.

## Lo que aun impide declarar el port completo

Queda integrar estas transformaciones con getSlbInfo, las autorizaciones de la
sesion y las fuentes de TV/VOD; comprobar la respuesta real, seleccionar el
transporte compatible y adaptar cabeceras y reproduccion. Las rutas de proxy,
formatos propietarios, renovacion de licencias y posibles funciones P2P no se
han sustituido por una implementacion nativa comprobada.

La revision automatica rechazo la comprobacion con el servicio real por envio
de identificadores y posible material de sesion a destinos externos sin una
autorizacion explicita para esos destinos. No se considera validada esa conexion
ni se intenta repetir por otra via. La autorizacion solicitada se limita a una
instalacion de prueba y a consultas/pruebas del servicio; no requiere contrasenas
del usuario. El programa de prueba esta preparado, pero su verificacion queda
pendiente de esa autorizacion.

El resto de este documento describe el estado previo a este avance.

---

# Estado de Ranger en 0.6

El fallo informado no es un rechazo de activacion: el servicio entrega una fuente
que el cliente no sabe resolver. Se revisaron los puntos de integracion en la APK:

- com/titan/ranger/NativeJni carga ranger-jni y c++_shared. Call es nativo.
- za/g transforma cdn_list de getSlbInfo en Entry: main_addr, marcas, estrategia,
  auths, link, sign_type, prioridades y parametros. Las URL de esa lista son
  entradas del resolvedor; no se ha demostrado que sean enlaces finales de video.
- h8/v3, h8/f2 y ud/n1 entregan Media con playCode/contentId, licencia, calidad,
  audio y formatos a Ranger. xa/p0 extrae campos de las cadenas de parametros;
  esto no reconstruye las firmas o la resolucion de la biblioteca.
- Las operaciones JNI incluyen Init, PrepareProgram, SetMedia y notificaciones.
  El estado devuelve play_url, consumido por el reproductor Android.
- La muestra ARM64 es un ELF compartido de Android de 7.9 MiB con dependencias
  liblog, libz, libc++_shared, libm, libdl y libc. Usa ABI/JNI de Android, ausentes
  en Horizon. No basta renombrar, copiar o enlazar el binario con devkitA64.

No se reconstruyo la transformacion de entrada autorizada a play_url. Tampoco
se demostro que main_addr, url_list o un parametro de licencia puedan utilizarse
directamente como medio. Por eso no se implementa una concatenacion especulativa.

Para resolver esta parte es necesario reconstruir el protocolo de la biblioteca
o disponer de su codigo/documentacion, y contrastar entrada/salida de una
reproduccion autorizada en Android. No se incluyen en el ZIP bibliotecas
propietarias, tokens de sesion ni respuestas reales del usuario.

La nueva navegacion VOD y la busqueda usan las APIs Java observadas y pueden
verificarse independientemente. No se presentan como una solucion de Ranger.

0.8: la peticion SLB GET sin cuerpo incluye Content-Type: application/octet-stream,
segun RequestSLB ARM64. Se verifico enum 1=GET en la tabla original de metodos.
El constructor Live completo coincide con dos autorizaciones sinteticas.
Esto no confirma por si solo que el servidor real deje de devolver HTTP400.

0.9: AssetList.type controla pelicula/serie; programType es metadata. Se agregan
App, App-Version y Ranger-Id al SLB, segun mapa nativo en SetEnv. Ranger-Id
comprobado contra tres ejecuciones ARM64 sinteticas y decoder Python independiente.
No se ha confirmado que 0.9 resuelva el HTTP400 real; diagnostico estructural
sin credenciales ni respuestas crudas disponible para investigar el resultado.

0.10: el decoder ARM64 0x6790ac admite terminacion '='. El port confundia
la codificacion sin relleno de solicitudes con el formato permitido de respuestas.
Se acepta relleno Base64 canonico y CRLF posterior al relleno; AES y PKCS7 siguen
verificandose. libcurl procesa gzip; diagnostico distingue HTML/JSON y cuenta
caracteres no admitidos sin conservar el cuerpo de la respuesta. TV real pendiente.

0.11 implementa Content-Auth de TV SLB: BuildReq 0x46de48 toma Uri::resource()
(path/query/fragment) y lo firma mediante 0x6752e0; no envia el auth de getSlbInfo.
Con token= no vacio agrega sign2_method=sign_o3, instance y start_moment (ms),
y firma el sufijo desde token= + campos agregados + salt binario de 21 bytes con
el MD5 modificado original. Cuatro vectores ARM64 validan el resultado.
Agrega las cabeceras App/App-Version/Ranger-Id/Pragma del original al video.
Se mantiene la URL media_url original. Esta es la peticion inicial: falta portar
el proxy y la renovacion/firma individual de segmentos/redirects. TV real pendiente.

0.12: GetMediaInfo/0x47db34 conserva la autoridad y query de media_url y cambia
el path mediante 0x480314/0x4804f4 a /live/<Media.name>.ts (o .mp4). Media.name
es playCode y AVFormat selecciona extension. Content-Auth firmado conserva el
recurso ORIGINAL del servidor; no se firma el path nuevo. Cuatro vectores ARM64
comprueban el constructor. La regla se aplica solo a TV SLB cdn_type=1.
