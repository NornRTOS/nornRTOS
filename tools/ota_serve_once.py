#!/usr/bin/env python3
"""Serves one file as /update.nota exactly once, then 404 (and keeps running so that the
board's later requests get a clean "no update"). For examples/ota and tools/ota_test.sh.

    tools/ota_serve_once.py PORT FILE"""
import sys, http.server

class Handler(http.server.BaseHTTPRequestHandler):
    served = False
    def do_GET(self):
        if self.path == "/update.nota" and not Handler.served:
            Handler.served = True
            data = open(sys.argv[2], "rb").read()
            self.send_response(200); self.send_header("Content-Type", "application/octet-stream")
            self.send_header("Content-Length", str(len(data))); self.end_headers()
            self.wfile.write(data)
            sys.stderr.write("served %d bytes\n" % len(data))
        else:
            self.send_response(404); self.send_header("Content-Length", "0"); self.end_headers()
            sys.stderr.write("404 %s\n" % self.path)
    def log_message(self, *a):
        pass

http.server.HTTPServer(("", int(sys.argv[1])), Handler).serve_forever()
