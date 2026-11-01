"""Serve the local inspection viewer: python Tools/serve_proofs.py [8080]."""
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
class Handler(SimpleHTTPRequestHandler):
    def __init__(self,*args,**kwargs):super().__init__(*args,directory=str(ROOT),**kwargs)
    def do_GET(self):
        if self.path=='/':
            self.send_response(302);self.send_header('Location','/Proofs/');self.end_headers();return
        super().do_GET()
ThreadingHTTPServer(('0.0.0.0',int(sys.argv[1]) if len(sys.argv)>1 else 8080),Handler).serve_forever()
