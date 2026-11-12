#!/usr/bin/env python3
"""
Control / update the wash & cure over WiFi via the ESP8266 bridge.

  washcure.py status                     print status JSON
  washcure.py cmd "MODE WASH" "START"    run console commands
  washcure.py flash-unit fw/wnc_fw.bin   update the SWM190 firmware over WiFi (UART FL commands)
  washcure.py flash-esp  esp_bridge/build/esp_bridge.ino.bin   update the ESP8266 (HTTP OTA)
  washcure.py console                    interactive raw console (TCP 23); Ctrl-C to quit

Host defaults to washcure.local; override with --host IP.
"""
import argparse, json, socket, sys, threading, time, urllib.parse, urllib.request

def http(host, path, data=None, ctype=None, timeout=10):
    req = urllib.request.Request(f"http://{host}{path}", data=data)
    if ctype:
        req.add_header("Content-Type", ctype)
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.read().decode(errors="replace")

def upload(host, path, field, filename, timeout):
    boundary = "----washcure" + str(int(time.time()))
    body = open(filename, "rb").read()
    payload = (f"--{boundary}\r\nContent-Disposition: form-data; name=\"{field}\"; filename=\"fw.bin\"\r\n"
               f"Content-Type: application/octet-stream\r\n\r\n").encode() + body + f"\r\n--{boundary}--\r\n".encode()
    return http(host, path, payload, f"multipart/form-data; boundary={boundary}", timeout)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("action", choices=["status", "cmd", "flash-unit", "flash-esp", "console"])
    ap.add_argument("args", nargs="*")
    ap.add_argument("--host", default="washcure.local")
    a = ap.parse_args()
    host = socket.gethostbyname(a.host)

    if a.action == "status":
        print(json.dumps(json.loads(http(host, "/api/status")), indent=2))
    elif a.action == "cmd":
        for c in a.args:
            print(f"> {c}\n" + http(host, "/api/cmd?c=" + urllib.parse.quote(c)).rstrip())
    elif a.action == "flash-unit":
        print(upload(host, "/api/mcu/update", "file", a.args[0], 300).rstrip())
    elif a.action == "flash-esp":
        print(upload(host, "/update", "image", a.args[0], 120).rstrip()[:200])
    elif a.action == "console":
        s = socket.create_connection((host, 23))
        def rx():
            while True:
                d = s.recv(4096)
                if not d:
                    break
                sys.stdout.write(d.decode(errors="replace")); sys.stdout.flush()
        threading.Thread(target=rx, daemon=True).start()
        try:
            for line in sys.stdin:
                s.sendall(line.rstrip("\n").encode() + b"\r\n")
        except KeyboardInterrupt:
            pass

if __name__ == "__main__":
    main()
