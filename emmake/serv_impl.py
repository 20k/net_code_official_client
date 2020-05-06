#import BaseHTTPServer, SimpleHTTPServer


#SimpleHTTPServer.SimpleHTTPRequestHandler.extensions_map['.wasm'] =  'application/wasm'
#SimpleHTTPServer.SimpleHTTPRequestHandler.extensions_map['.js'] =  'application/ecmascript'
#port = 8000
#httpd = BaseHTTPServer.HTTPServer(('localhost', 8000), SimpleHTTPServer.SimpleHTTPRequestHandler)
#httpd.serve_forever()

import http.server
from http.server import HTTPServer, BaseHTTPRequestHandler
import socketserver

PORT = 8080

Handler = http.server.SimpleHTTPRequestHandler

Handler.extensions_map={
        '.manifest': 'text/cache-manifest',
	'.html': 'text/html',
        '.png': 'image/png',
	'.jpg': 'image/jpg',
	'.svg':	'image/svg+xml',
	'.css':	'text/css',
	'.js':	'application/x-javascript',
	'': 'application/octet-stream', # Default
    }

httpd = socketserver.TCPServer(("", PORT), Handler)

print("serving at port", PORT)
httpd.serve_forever()