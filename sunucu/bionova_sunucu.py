#!/usr/bin/env python3
"""BioNova sunucusu: sayfaları yayınlar ve astronot/yer kontrol ekranları arasında
mesaj aktarır. Tek port, yalnızca Python standart kütüphanesi.

  python3 bionova_sunucu.py [PORT]        (varsayılan 8765)

Adresler:  /  ·  /astronot  ·  /yerkontrol  ·  /panel  ·  WebSocket: /ws
Sayfa dosyaları bu dosyayla aynı klasörde olmalı.
Başka hiçbir servise, porta ya da web sunucusu ayarına dokunmaz.
"""
import base64, gzip, hashlib, os, socket, struct, sys, threading, time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8765
HERE = os.path.dirname(os.path.abspath(__file__))
GUID = b"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
PAGES = {"/astronot": "BioNova_Astronot.html", "/yerkontrol": "BioNova_YerKontrol.html", "/panel": "BioNova_Panel.html"}
INDEX = """<!doctype html><html lang="tr"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>BioNova</title><style>body{margin:0;min-height:100vh;display:grid;place-items:center;background:#0E0F11;color:#EDEEF0;font:16px system-ui,sans-serif}
main{display:grid;gap:14px;width:min(420px,90vw)}h1{margin:0 0 8px;font-size:40px;letter-spacing:.02em}a{display:block;padding:18px 20px;border:1px solid #33363C;border-radius:10px;background:#151619;color:#EDEEF0;text-decoration:none}
a:hover{border-color:#7FD1A8}small{color:#9B9FA7}</style></head><body><main><h1>BIONOVA</h1>
<a href="/astronot">Astronot ekranı<br><small>Astronaut monitor</small></a>
<a href="/yerkontrol">Yer kontrol ekranı<br><small>Ground control</small></a>
<a href="/panel">İki ekran tek sayfada<br><small>Both views</small></a>
<small>Bağlı ekran: {n}</small></main></body></html>"""

cache = {}
def load(name):
    p = os.path.join(HERE, name)
    st = os.stat(p)
    c = cache.get(name)
    if not c or c[0] != st.st_mtime:
        raw = open(p, "rb").read()
        c = (st.st_mtime, raw, gzip.compress(raw, 6))
        cache[name] = c
    return c

clients, lock = set(), threading.Lock()

def recv_exact(c, n):
    b = b""
    while len(b) < n:
        part = c.recv(n - len(b))
        if not part:
            raise ConnectionError
        b += part
    return b

def frame(op, payload):
    n = len(payload)
    if n < 126: h = struct.pack("!BB", 0x80 | op, n)
    elif n < 65536: h = struct.pack("!BBH", 0x80 | op, 126, n)
    else: h = struct.pack("!BBQ", 0x80 | op, 127, n)
    return h + payload

def read_msg(c):
    buf, first = b"", None
    while True:
        b1, b2 = recv_exact(c, 2)
        fin, op, n = b1 & 0x80, b1 & 0x0F, b2 & 0x7F
        if n == 126: n = struct.unpack("!H", recv_exact(c, 2))[0]
        elif n == 127: n = struct.unpack("!Q", recv_exact(c, 8))[0]
        if n > 1 << 20: raise ConnectionError
        mask = recv_exact(c, 4) if b2 & 0x80 else b"\0\0\0\0"
        data = bytes(x ^ mask[i % 4] for i, x in enumerate(recv_exact(c, n)))
        if op == 8: raise ConnectionError
        if op == 9: c.sendall(frame(10, data)); continue
        if op == 10: continue
        if op in (1, 2): first = op
        buf += data
        if fin: return first or 1, buf

def drop(c):
    with lock:
        if c in clients:
            clients.discard(c)
            print(time.strftime("%H:%M:%S"), f"[-] ekran ayrıldı · bağlı: {len(clients)}", flush=True)
    try: c.close()
    except OSError: pass

def ws_loop(c, addr):
    with lock: clients.add(c)
    print(time.strftime("%H:%M:%S"), f"[+] ekran bağlandı {addr[0]} · bağlı: {len(clients)}", flush=True)
    try:
        while True:
            op, data = read_msg(c)
            f = frame(op, data)
            with lock: targets = [x for x in clients if x is not c]
            for t in targets:
                try: t.sendall(f)
                except OSError: drop(t)
    except (ConnectionError, OSError, ValueError):
        pass
    finally:
        drop(c)

def http_reply(c, code, ctype, body, extra=b""):
    c.sendall(b"HTTP/1.1 " + code + b"\r\nContent-Type: " + ctype + b"\r\nContent-Length: " + str(len(body)).encode() +
              b"\r\nCache-Control: no-cache\r\nConnection: close\r\n" + extra + b"\r\n" + body)

def serve(c, addr):
    try:
        c.settimeout(30)
        data = b""
        while b"\r\n\r\n" not in data:
            part = c.recv(8192)
            if not part or len(data) > 32768: raise ConnectionError
            data += part
        head = data.split(b"\r\n\r\n", 1)[0].decode("latin-1")
        lines = head.split("\r\n")
        method, path = (lines[0].split(" ") + ["", ""])[:2]
        hdr = {l.split(":", 1)[0].strip().lower(): l.split(":", 1)[1].strip() for l in lines[1:] if ":" in l}
        path = path.split("?", 1)[0]
        if hdr.get("upgrade", "").lower() == "websocket":
            acc = base64.b64encode(hashlib.sha1(hdr["sec-websocket-key"].encode() + GUID).digest())
            c.sendall(b"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " + acc + b"\r\n\r\n")
            c.settimeout(None)
            ws_loop(c, addr)
            return
        if method not in ("GET", "HEAD"):
            http_reply(c, b"405 Method Not Allowed", b"text/plain", b"")
        elif path in ("/", "/index.html"):
            with lock: n = len(clients)
            http_reply(c, b"200 OK", b"text/html; charset=utf-8", INDEX.replace("{n}", str(n)).encode())
        elif path.rstrip("/") in PAGES:
            _, raw, gz = load(PAGES[path.rstrip("/")])
            if "gzip" in hdr.get("accept-encoding", ""):
                http_reply(c, b"200 OK", b"text/html; charset=utf-8", gz if method == "GET" else b"", b"Content-Encoding: gzip\r\n")
            else:
                http_reply(c, b"200 OK", b"text/html; charset=utf-8", raw if method == "GET" else b"")
        else:
            http_reply(c, b"404 Not Found", b"text/plain; charset=utf-8", "Bulunamadı".encode())
        c.close()
    except (ConnectionError, OSError, ValueError, KeyError):
        try: c.close()
        except OSError: pass

def main():
    for n in PAGES.values():
        if not os.path.exists(os.path.join(HERE, n)):
            print("Eksik dosya:", n, "(bionova_sunucu.py ile aynı klasöre koy)")
    for n in PAGES.values():
        if os.path.exists(os.path.join(HERE, n)): load(n)
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("0.0.0.0", PORT)); srv.listen(32)
    print(f"BioNova sunucusu hazır · http://<sunucu-ip>:{PORT}/", flush=True)
    while True:
        c, addr = srv.accept()
        threading.Thread(target=serve, args=(c, addr), daemon=True).start()

if __name__ == "__main__":
    main()
