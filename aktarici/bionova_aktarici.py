#!/usr/bin/env python3
"""BioNova aktarıcı: ESP32 kartı yokken astronot ve yer kontrol ekranlarını
aynı Wi-Fi / hotspot üzerinden birbirine bağlar.

ESP32 ile aynı protokolü konuşur: ws://<bu bilgisayarın IP'si>:81
Bir istemciden gelen her metin mesajını diğer tüm istemcilere iletir.
Gecikme (ISS/Ay/Mars) panelin yer kontrol tarafında uygulanır.
Yalnızca Python standart kütüphanesi kullanılır; internet gerekmez.
"""
import base64, hashlib, os, socket, struct, sys, threading
from urllib.parse import urlparse, parse_qs

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 81
GUID = b"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
KEY = os.environ.get("BIONOVA_KEY", "")  # boş değilse istemci ws://host:port/?key=... ile bağlanmalı
clients, lock = set(), threading.Lock()


def recv_exact(c, n):
    b = b""
    while len(b) < n:
        part = c.recv(n - len(b))
        if not part:
            raise ConnectionError
        b += part
    return b


def handshake(c):
    data = b""
    while b"\r\n\r\n" not in data:
        part = c.recv(4096)
        if not part or len(data) > 16384:
            raise ConnectionError
        data += part
    key = None
    path = data.split(b"\r\n", 1)[0].split(b" ")[1].decode("latin-1") if b" " in data.split(b"\r\n", 1)[0] else "/"
    if KEY and parse_qs(urlparse(path).query).get("key", [""])[0] != KEY:
        c.sendall(b"HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n")
        raise ConnectionError
    for line in data.split(b"\r\n"):
        if line.lower().startswith(b"sec-websocket-key:"):
            key = line.split(b":", 1)[1].strip()
    if not key:
        c.sendall(b"HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\n\r\nBioNova aktarici calisiyor.\n")
        raise ConnectionError
    acc = base64.b64encode(hashlib.sha1(key + GUID).digest())
    c.sendall(b"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
              b"Sec-WebSocket-Accept: " + acc + b"\r\n\r\n")


def frame(op, payload):
    n = len(payload)
    if n < 126:
        h = struct.pack("!BB", 0x80 | op, n)
    elif n < 65536:
        h = struct.pack("!BBH", 0x80 | op, 126, n)
    else:
        h = struct.pack("!BBQ", 0x80 | op, 127, n)
    return h + payload


def read_msg(c):
    buf, first_op = b"", None
    while True:
        b1, b2 = recv_exact(c, 2)
        fin, op = b1 & 0x80, b1 & 0x0F
        n = b2 & 0x7F
        if n == 126:
            n = struct.unpack("!H", recv_exact(c, 2))[0]
        elif n == 127:
            n = struct.unpack("!Q", recv_exact(c, 8))[0]
        if n > 1 << 20:
            raise ConnectionError
        mask = recv_exact(c, 4) if b2 & 0x80 else b"\0\0\0\0"
        data = bytes(x ^ mask[i % 4] for i, x in enumerate(recv_exact(c, n)))
        if op == 8:
            raise ConnectionError
        if op == 9:
            c.sendall(frame(10, data)); continue
        if op == 10:
            continue
        if op in (1, 2):
            first_op = op
        buf += data
        if fin:
            return first_op or 1, buf


def broadcast(src, op, data):
    f = frame(op, data)
    with lock:
        targets = [x for x in clients if x is not src]
    for t in targets:
        try:
            t.sendall(f)
        except OSError:
            drop(t)


def drop(c):
    with lock:
        if c in clients:
            clients.discard(c)
            print(f"[-] ayrıldı · bağlı ekran: {len(clients)}", flush=True)
    try:
        c.close()
    except OSError:
        pass


def serve(c, addr):
    try:
        c.settimeout(30)
        handshake(c)
        c.settimeout(None)
        with lock:
            clients.add(c)
        print(f"[+] bağlandı {addr[0]} · bağlı ekran: {len(clients)}", flush=True)
        while True:
            op, data = read_msg(c)
            broadcast(c, op, data)
    except (ConnectionError, OSError, ValueError):
        pass
    finally:
        drop(c)


def local_ips():
    ips = set()
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("10.255.255.255", 1)); ips.add(s.getsockname()[0]); s.close()
    except OSError:
        pass
    try:
        for i in socket.gethostbyname_ex(socket.gethostname())[2]:
            if not i.startswith("127."):
                ips.add(i)
    except OSError:
        pass
    return sorted(ips) or ["127.0.0.1"]


def main():
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        srv.bind(("0.0.0.0", PORT))
    except OSError as e:
        print(f"Port {PORT} açılamadı: {e}"); sys.exit(1)
    srv.listen(16)
    print("BioNova aktarıcı çalışıyor.")
    print("Her iki bilgisayarda panelde ESP32 düğmesine bas ve şu adresi yaz:")
    for ip in local_ips():
        print(f"   {ip}" + ("" if PORT == 81 else f":{PORT}"))
    print("Bu bilgisayarın kendi paneli için: 127.0.0.1   ·  Kapatmak: Ctrl+C\n", flush=True)
    try:
        while True:
            c, addr = srv.accept()
            threading.Thread(target=serve, args=(c, addr), daemon=True).start()
    except KeyboardInterrupt:
        print("\nKapatıldı.")


if __name__ == "__main__":
    main()
