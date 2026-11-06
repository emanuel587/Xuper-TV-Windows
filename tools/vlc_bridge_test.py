#!/usr/bin/env python3
"""Temporary loopback relay to test an authorized Xuper live source in VLC.

No source URL or authorization header is written to disk or printed.
Standard library only. Run: py tools/vlc_bridge_test.py
"""
from __future__ import annotations

import getpass
import http.server
import secrets
import shutil
import subprocess
import threading
import urllib.error
import urllib.parse
import urllib.request

URLS: dict[str, str] = {}
URLS_LOCK = threading.Lock()
TOKEN = secrets.token_urlsafe(24)
SOURCE = ""
UPSTREAM_HEADERS: dict[str, str] = {}


def add_url(url: str) -> str:
    key = secrets.token_urlsafe(12)
    with URLS_LOCK:
        URLS[key] = url
    return key


def rewrite_playlist(text: str, base_url: str) -> str:
    output = []
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped:
            output.append(line)
        elif stripped.startswith("#"):
            # Rewrite URI="..." attributes used by keys, maps and alternate tracks.
            def replace_uri(match):
                absolute = urllib.parse.urljoin(base_url, match.group(2))
                return match.group(1) + "/".join(("", TOKEN, add_url(absolute)))
            import re
            line = re.sub(r'(URI=")([^"]+)(")', lambda m: m.group(1) + "/".join(("", TOKEN, add_url(urllib.parse.urljoin(base_url, m.group(2))))) + m.group(3), line)
            output.append(line)
        else:
            absolute = urllib.parse.urljoin(base_url, stripped)
            output.append("/".join(("", TOKEN, add_url(absolute))))
    return "\n".join(output) + "\n"


class RelayHandler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, *_args):
        # Do not log local tokens, upstream URLs, or headers.
        pass

    def do_GET(self):
        self._relay("GET")

    def do_HEAD(self):
        self._relay("HEAD")

    def _relay(self, method: str):
        prefix = f"/{TOKEN}/"
        if not self.path.startswith(prefix):
            self.send_error(404)
            return
        key = self.path[len(prefix):].split("?", 1)[0]
        if key == "root":
            upstream_url = SOURCE
        else:
            with URLS_LOCK:
                upstream_url = URLS.get(key)
        if not upstream_url:
            self.send_error(404)
            return

        headers = dict(UPSTREAM_HEADERS)
        for name in ("Range", "If-Range", "If-None-Match"):
            value = self.headers.get(name)
            if value:
                headers[name] = value

        request = urllib.request.Request(upstream_url, headers=headers, method=method)
        try:
            response = urllib.request.urlopen(request, timeout=25)
        except urllib.error.HTTPError as exc:
            response = exc
        except Exception:
            self.send_error(502, "No se pudo conectar con el origen")
            return

        status = getattr(response, "status", 200)
        content_type = response.headers.get("Content-Type", "")
        is_playlist = ("mpegurl" in content_type.lower()
                       or urllib.parse.urlsplit(upstream_url).path.lower().endswith(".m3u8"))

        if is_playlist and method == "GET" and status == 200:
            try:
                body = response.read(8 * 1024 * 1024 + 1)
            except Exception:
                response.close()
                self.send_error(502, "Error leyendo el manifiesto")
                return
            response.close()
            if len(body) > 8 * 1024 * 1024:
                self.send_error(502, "Manifiesto demasiado grande")
                return
            text = body.decode("utf-8-sig", errors="replace")
            body = rewrite_playlist(text, upstream_url).encode("utf-8")
            self.send_response(status)
            self.send_header("Content-Type", "application/vnd.apple.mpegurl")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Connection", "close")
            self.end_headers()
            try:
                self.wfile.write(body)
            except (BrokenPipeError, ConnectionResetError):
                pass
            return

        self.send_response(status)
        if content_type:
            self.send_header("Content-Type", content_type)
        for name in ("Content-Range", "Accept-Ranges", "ETag", "Last-Modified"):
            value = response.headers.get(name)
            if value:
                self.send_header(name, value)
        length = response.headers.get("Content-Length")
        if length:
            self.send_header("Content-Length", length)
        self.send_header("Connection", "close")
        self.end_headers()
        if method == "GET":
            try:
                while True:
                    chunk = response.read(64 * 1024)
                    if not chunk:
                        break
                    self.wfile.write(chunk)
                    self.wfile.flush()
            except (BrokenPipeError, ConnectionResetError):
                pass
            except Exception:
                pass
        response.close()



def main():
    global SOURCE, UPSTREAM_HEADERS
    SOURCE = input("Pega la URL de reproducción autorizada (no se guardará): ").strip()
    parsed = urllib.parse.urlsplit(SOURCE)
    if parsed.scheme not in ("http", "https") or not parsed.hostname:
        print("La URL debe comenzar por http:// o https://")
        return

    print("\nCabeceras necesarias (deja el nombre vacío para terminar).")
    print("Los valores se capturan ocultos y no se guardan.")
    while True:
        name = input("Nombre de cabecera: ").strip()
        if not name:
            break
        if any(ch in name for ch in "\r\n:"):
            print("Nombre no válido.")
            continue
        value = getpass.getpass(f"Valor de {name}: ")
        if "\r" in value or "\n" in value:
            print("Valor no válido.")
            continue
        UPSTREAM_HEADERS[name] = value

    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), RelayHandler)
    server.daemon_threads = True
    local_url = f"http://127.0.0.1:{server.server_port}/{TOKEN}/root"
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()

    vlc = shutil.which("vlc") or shutil.which("vlc.exe")
    print("\nPuente local iniciado. La URL y las cabeceras originales permanecen en memoria.")
    print("Prueba VLC con esta dirección local:")
    print(local_url)
    if vlc:
        subprocess.Popen([vlc, local_url])
        print("VLC se abrió.")
    else:
        print("No encontré vlc.exe en PATH. Copia la URL local y ábrela manualmente en VLC.")
    print("Pulsa Ctrl+C para cerrar el puente.")
    try:
        thread.join()
    except KeyboardInterrupt:
        pass
    finally:
        server.shutdown()
        server.server_close()
        with URLS_LOCK:
            URLS.clear()
        UPSTREAM_HEADERS.clear()
        SOURCE = ""
        print("\nPuente cerrado; datos temporales limpiados.")


if __name__ == "__main__":
    main()
