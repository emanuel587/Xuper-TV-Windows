import json
import os
import shutil
import subprocess
import tkinter as tk
from tkinter import ttk, messagebox

ROOT = os.path.dirname(os.path.abspath(__file__))
BIN = os.path.join(ROOT, "build-host", "python_service_probe.exe")
CONFIG = os.path.join(ROOT, "service.json")

class Probe:
    def __init__(self, root):
        self.root = root
        self.proc = None
        self.items = []
        self.last_source = None
        root.title("Prueba Xuper - Python")
        root.geometry("820x520")
        root.minsize(650, 400)

        bar = ttk.Frame(root, padding=10)
        bar.pack(fill="x")
        ttk.Label(bar, text="Motor de servicio original + Ranger + mpv", font=("Segoe UI", 11, "bold")).pack(side="left")
        self.connect_btn = ttk.Button(bar, text="Conectar y cargar categorías", command=self.connect)
        self.connect_btn.pack(side="right")

        body = ttk.Frame(root, padding=(10, 0, 10, 10))
        body.pack(fill="both", expand=True)
        left = ttk.Frame(body)
        left.pack(side="left", fill="both", expand=True)
        right = ttk.Frame(body)
        right.pack(side="left", fill="both", expand=True, padx=(10, 0))
        ttk.Label(left, text="Categorías / secciones").pack(anchor="w")
        self.categories = tk.Listbox(left, font=("Segoe UI", 10), exportselection=False)
        self.categories.pack(fill="both", expand=True, pady=(5, 0))
        self.categories.bind("<Double-Button-1>", self.open_selected)
        ttk.Label(right, text="Canales").pack(anchor="w")
        self.channels = tk.Listbox(right, font=("Segoe UI", 10), exportselection=False)
        self.channels.pack(fill="both", expand=True, pady=(5, 0))
        self.channels.bind("<Double-Button-1>", self.play_selected)

        footer = ttk.Frame(root, padding=10)
        footer.pack(fill="x")
        ttk.Button(footer, text="Abrir categoría", command=self.open_selected).pack(side="left")
        ttk.Button(footer, text="Reproducir canal", command=self.play_selected).pack(side="left", padx=6)
        self.status = tk.StringVar(value="Listo. Se requiere service.json junto al programa.")
        ttk.Label(root, textvariable=self.status, relief="sunken", anchor="w", padding=6).pack(fill="x")

    def start_engine(self):
        if self.proc and self.proc.poll() is None:
            return True
        if not os.path.isfile(BIN):
            messagebox.showerror("Falta el motor", "No existe build-host/python_service_probe.exe.\nCompila con: mingw32-make -f Makefile.host python-probe")
            return False
        if not os.path.isfile(CONFIG):
            messagebox.showerror("Falta configuración", "Copia tu service.json autorizado junto a esta app. No lo compartas.")
            return False
        self.proc = subprocess.Popen([BIN, CONFIG], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     stderr=subprocess.DEVNULL, text=True, encoding="utf-8", bufsize=1)
        return True

    def request(self, cmd, **kwargs):
        if not self.start_engine():
            return None
        self.proc.stdin.write(json.dumps({"cmd": cmd, **kwargs}, ensure_ascii=False) + "\n")
        self.proc.stdin.flush()
        line = self.proc.stdout.readline()
        if not line:
            self.status.set("El motor terminó inesperadamente.")
            return None
        try:
            return json.loads(line)
        except json.JSONDecodeError:
            self.status.set("Respuesta local no reconocida.")
            return None

    def connect(self):
        self.status.set("Conectando con el servicio y cargando catálogo...")
        self.root.update_idletasks()
        data = self.request("bootstrap")
        self.show_catalog(data, self.categories)
        if data:
            self.status.set(data.get("status", "Conexión completada."))

    def show_catalog(self, data, target):
        if not data:
            return
        if not data.get("ok"):
            self.status.set(data.get("status", "No se pudo cargar."))
            messagebox.showwarning("Conexión", self.status.get())
            return
        self.items = data.get("items", [])
        target.delete(0, tk.END)
        for item in self.items:
            target.insert(tk.END, f'{item.get("title","Sin nombre")}  [{item.get("kind","")}]')
        self.status.set(data.get("status", "Catálogo recibido."))

    def open_selected(self, event=None):
        sel = self.categories.curselection()
        if not sel:
            return
        item = self.items[sel[0]]
        if item.get("kind") == "category":
            data = self.request("channels", column_id=item.get("column_id", -1))
        else:
            data = self.request("browse", index=item["index"])
        self.show_catalog(data, self.channels)
        if data and data.get("ok"):
            self.channel_items = data.get("items", [])
            self.status.set(data.get("status", "Canales cargados."))

    def play_selected(self, event=None):
        items = getattr(self, "channel_items", [])
        sel = self.channels.curselection()
        if not sel or sel[0] >= len(items):
            return
        item = items[sel[0]]
        self.status.set("Solicitando fuente autorizada a Ranger...")
        self.root.update_idletasks()
        data = self.request("source", index=item["index"])
        if not data:
            return
        if not data.get("ok"):
            self.status.set(data.get("status", "No se obtuvo fuente."))
            messagebox.showwarning("Ranger", self.status.get())
            return
        player = shutil.which("mpv") or shutil.which("mpv.exe")
        if not player:
            messagebox.showerror("Falta mpv", "Instala mpv para Windows y agrega mpv.exe al PATH.")
            return
        headers = ",".join(f'{h["name"]}: {h["value"]}' for h in data.get("headers", []))
        args = [player, "--force-window=yes", "--title=Xuper Python Probe"]
        if headers:
            args.append("--http-header-fields=" + headers)
        args.append(data["source"])
        subprocess.Popen(args, cwd=ROOT)
        self.status.set("Fuente autorizada entregada a mpv. Comprueba imagen y sonido.")

    def close(self):
        if self.proc and self.proc.poll() is None:
            try:
                self.request("quit")
                self.proc.wait(timeout=2)
            except Exception:
                self.proc.kill()
        self.root.destroy()

if __name__ == "__main__":
    window = tk.Tk()
    app = Probe(window)
    window.protocol("WM_DELETE_WINDOW", app.close)
    window.mainloop()
