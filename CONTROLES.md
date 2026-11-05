# Navegacion del catalogo 0.14

Consulta README.md: cuadrilla y menu lateral; X busca todo; R3 o boton tactil
guarda/quita canales; Favoritos en Inicio. Menos abre donaciones. Y vuelve
a Inicio; el catalogo se carga al iniciar.

# Reproductor 0.10

X o + abre el menu. A elige, B vuelve y arriba/abajo recorre la lista.
Izquierda/derecha ajusta velocidad, volumen, sincronizacion y tamano.
Con el menu cerrado A pausa, B sale, izquierda/derecha desplaza 10 segundos,
ZL/ZR desplaza 60 segundos y L/R cambia episodio/canal.

| Funcion | Comportamiento implementado |
| --- | --- |
| Pausa y progreso | Pausa/continua y muestra tiempo, duracion y barra |
| Adelantar/retroceder | Desplazamiento exacto de 10/60 s si el medio admite seek |
| Audio/idioma | Lista las pistas y sus idiomas; Y cambia rapidamente |
| Subtitulos | Pistas internas, desactivar, tamano y desfase; SRT HTTPS de la API |
| Calidad/fuente | Lista las fuentes autorizadas; solicita la seleccion y conserva tiempo, velocidad, volumen y pausa |
| Episodios/canales | Selector y L/R; la nueva entrada obtiene su propia autorizacion |
| Siguiente episodio | Automatico al finalizar una serie; se puede desactivar |
| Velocidad | 0.5, 0.75, 1, 1.25, 1.5, 1.75 y 2x con correccion de tono |
| Pantalla | Original, 16:9, 4:3 y rellenar |
| Sonido | Volumen, silenciar y desfase de audio |
| Imagen | Ajuste de brillo y datos del video |
| Continuar | Guarda posicion por contentId y retoma al volver a abrir |
| Reiniciar | Vuelve a cero cuando el medio permite seek |

El menu muestra diez filas a la vez y el contador al pie indica las restantes.
Al cambiar calidad el video se vuelve a abrir con una autorizacion nueva;
puede haber una espera. Los idiomas disponibles pueden estar dentro del archivo
o ser fuentes distintas: revisa Audio y Calidad/fuente.

SRT: descarga directa HTTPS sin cabeceras de sesion/medio, con verificacion TLS
y MD5 si el servicio proporciona la huella. Las rutas con redireccion o mecanismos
adicionales de descarga quedan pendientes. No se simula una opcion que la fuente
no entrega.

La APK tambien tiene selector de motores Android, feedback al proveedor y otras
funciones de sus servicios. Esas funciones, su interfaz exacta y los transportes
Ranger pendientes no estan completos en el port. El render actual es software
640x360. La seleccion interna de variantes HLS queda en automatico de mpv.

En PC de desarrollo: Espacio/Enter=A, X/Tab=menu, Escape=B, Y=audio,
S=subtitulos, [ y ]=velocidad, F=pantalla, M=silencio, J/L=60 s,
PageUp/PageDown=episodio anterior/siguiente.
