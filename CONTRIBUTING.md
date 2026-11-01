# Colaborar

1. Haz un fork y crea una rama para tu cambio.
2. Describe el problema y el comportamiento esperado en un issue o pull request.
3. Conserva el renderer SDL durante la reproduccion y al regresar al catalogo.
   No reintroduzcas consoleInit o destruccion/recreacion de renderer entre videos.
4. Mantén las verificaciones TLS, el aislamiento de cabeceras por destino y los
   rechazos del servicio. No añadas credenciales ni identidades a los registros.
5. Ejecuta las pruebas pertinentes; documenta modelo de Switch, version y tipo
   de arranque cuando hayas probado en hardware. Distingue pruebas locales de
   pruebas reales, y usa fixtures sinteticos para nuevos casos.
6. Envia el pull request con problema, cambio, validacion y limites conocidos.

No incluyas la APK, bibliotecas Android propietarias, streams del proveedor,
contrasenas, tokens, configuraciones reales ni archivos personales de usuarios.
El enlace de donacion es voluntario y no debe bloquear ninguna funcion.

Tu contribucion al codigo original debe ser compatible con su licencia MIT.
Conserva los creditos y las licencias independientes de terceros.
