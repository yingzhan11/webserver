#!/usr/bin/env python3

import os, sys, cgi
from datetime import datetime, timezone

method = os.environ["REQUEST_METHOD"]

def get_time_stamp():
    return datetime.now(timezone.utc).strftime("%a, %d %b %Y %H:%M:%S GMT")

content_len = 0
time_stamp = 0
status = ""
html_content = ""

if method == "GET":
    html_content = f"""
    <!DOCTYPE html>
    <html lang="en>
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
        <link rel="canonical" href="/index.html" />
        <link rel="shortcut icon" href="/favicon.ico">
        <title>CGI-GET</title>
        <link rel="stylesheet" href="/styles.css">
        <link
            href="https://fonts.googleapis.com/css2?family=DM+Mono:ital,wght@0,300;0,400;0,500;1,300;1,400;1,500&family=Major+Mono+Display&display=swap"
            rel="stylesheet">
    </head>
    <body>
        <div class="container-top">
            <header>
                <h2 class="site-logo">Webserver - Cgi</h2>
                <h5 class="site-slogan">This is when you finally understand why a URL starts with HTTP</h5>
                <nav>
                    <ul>
                        <li><a href="/index.html">Home</a></li>
                        <li><a href="/testcmd.html">TestCMD</a></li>
                        <li><a href="/upload.html">Upload</a></li>
                        <li><a href="/delete.html">Delete</a></li>
                        <li><a href="/cgiget.html">CGI-GET</a></li>
                    </ul>
                </nav>
            </header>
            <div class="main-content">
                <h3>CGI - get.py</h3>
                <p>Thank you for your time!</p>
                <div class="image-container images">
                    <img src="assets/hive-logo-dark.png" alt="people img">
                </div>
                """
    status = "200 OK"
else:
    status = "401 Unauthorized"
    html_content = """<!DOCTYPE html>\n<html lang="en">\n<body><p>Error: Unsupported HTTP method.</p></body></html>"""

html_content += """
        </div>
        <footer>
            <p>Webserver @ 2025</p>
        </footer>
    </div>
</body>
</html>
"""

time_stamp = get_time_stamp()
content_len = len(html_content.encode('utf-8'))

response_headers = (
    f"HTTP/1.1 {status}\r\n"
    f"Date: {time_stamp}\r\n"
    "Server: Webserv/1.0.0 (Linux)\r\n"
    "Connection: keep-alive\r\n"
    "Content-Type: text/html; charset=utf-8\r\n"
    f"Content-Length: {content_len}\r\n"
    "\r\n"
)

sys.stdout.write(response_headers)
sys.stdout.write(html_content)
sys.stdout.flush()

sys.exit(0)
