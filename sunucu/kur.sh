#!/usr/bin/env bash
# BioNova kurulum betiği · Ubuntu/Debian · diğer siteye dokunmaz (sadece 8765 portu)
set -e
PORT=8765
DIR="$(cd "$(dirname "$0")" && pwd)"
U="$(id -un)"
echo "== BioNova kuruluyor: $DIR (kullanıcı: $U, port: $PORT)"
command -v python3 >/dev/null || { echo "python3 yok: sudo apt install -y python3"; exit 1; }
if ss -ltn 2>/dev/null | grep -q ":$PORT "; then
  if systemctl is-active --quiet bionova 2>/dev/null; then echo "Eski BioNova servisi yeniden başlatılacak."; else echo "HATA: $PORT portunu başka bir program kullanıyor. Durduruldu."; exit 1; fi
fi
sed "s#/home/KULLANICI/bionova#$DIR#g; s#User=KULLANICI#User=$U#" "$DIR/bionova.service" | sudo tee /etc/systemd/system/bionova.service >/dev/null
sudo systemctl daemon-reload
sudo systemctl enable --now bionova
sudo systemctl restart bionova
if command -v ufw >/dev/null && sudo ufw status | grep -q "Status: active"; then
  sudo ufw allow $PORT/tcp >/dev/null && echo "Güvenlik duvarında $PORT/tcp açıldı."
fi
sleep 2
if curl -fs -o /dev/null "http://127.0.0.1:$PORT/"; then echo "Sunucu çalışıyor."; else echo "Sunucu yanıt vermedi: journalctl -u bionova -n 30"; exit 1; fi
LAN=$(hostname -I | awk '{print $1}')
PUB=$(curl -4 -fs --max-time 5 ifconfig.me || echo "?")
echo
echo "Evdeki ağdan:   http://$LAN:$PORT/"
echo "Dışarıdan:      http://$PUB:$PORT/   (modemde $PORT → $LAN:$PORT yönlendirmesi gerekli)"
echo "Canlı kayıt:    journalctl -u bionova -f"
