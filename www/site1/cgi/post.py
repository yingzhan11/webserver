#!/usr/bin/env python3

import os, sys, cgi, cgitb
from datetime import datetime, timezone

cgitb.enable()

form = cgi.FieldStorage()
method = os.environ["REQUEST_METHOD"]
upload_dir = os.environ["UPLOAD_PATH"]

def get_time_stamp():
    return datetime.now(timezone.utc).strftime("%a, %d %b %Y %H:%M:%S GMT")

# Create upload directory if it does not exist
if not os.path.exists(upload_dir):
    os.makedirs(upload_dir)

content_len = 0
time_stamp = 0
html_content = ""
status = "200 OK"

# Check if the request is from curl
if 'USER_AGENT' in os.environ and 'curl' in os.environ['USER_AGENT']:
    status = "401 Unauthorized"
    html_content += """<!DOCTYPE html><html lang="en"><head><body><h3>Error: use curl.py for POST requests with curl</h3>"""
elif method == "POST":
    html_content = """
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
        <link rel="canonical" href="/index.html" />
        <link rel="shortcut icon" href="/favicon.ico">
        <title>Upload</title>
        <link rel="stylesheet" href="/styles.css">
        <link
            href="https://fonts.googleapis.com/css2?family=DM+Mono:ital,wght@0,300;0,400;0,500;1,300;1,400;1,500&family=Major+Mono+Display&display=swap"
            rel="stylesheet">
    </head>
    <body>
        <div class="container-top">
            <header>
                <h2 class="site-logo">Webserver - Upload</h2>
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
    """

    if "fileUpload" in form:
        file_item = form["fileUpload"]

        if isinstance(file_item, list):
            file_item = file_item[0]
        if file_item.filename:
            filename = os.path.basename(file_item.filename)
            filepath = os.path.join(upload_dir, filename)
            try:
                with open(filepath, "wb") as f:
                    while True:
                        chunk = file_item.file.read(1024)
                        if not chunk:
                            break
                        f.write(chunk)
                status = "200 OK"
                html_content += f"<h3>File '<b>{filename}</b>' successfully uploaded to <b>{filepath}</b>.</h3>"
            except Exception as e:
                status = "500 Internal Server Error"
                html_content += f"<h3>Error uploading the file: {e}</h3>"
        else:
            status = "400 Bad Request"
            html_content += "<h3>Error uploading the file: no file was provided.</h3>"
    else:
        status = "400 Bad Request"
        html_content += "<h3>No file was uploaded.</h3>"
else:
    status = "401 Unauthorized"
    html_content += """<!DOCTYPE html><html lang="en"><head><body><h3>Error: HTTP method not supported.</h3>"""

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
