import os
import socketserver
import sys

import http.server


class _Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, forced_content_type: str | None = None, **kwargs):
        self._forced_content_type = forced_content_type
        super().__init__(*args, **kwargs)

    def guess_type(self, path: str) -> str:
        if self._forced_content_type:
            return self._forced_content_type
        return super().guess_type(path)


def main() -> int:
    if len(sys.argv) < 3:
        raise SystemExit(
            "usage: http_server.py <directory> <port_file> [bind_host] [forced_content_type]"
        )

    directory = sys.argv[1]
    port_file = sys.argv[2]
    bind_host = sys.argv[3] if len(sys.argv) > 3 else "127.0.0.1"
    forced_content_type = sys.argv[4] if len(sys.argv) > 4 else None

    os.chdir(directory)
    handler = lambda *args, **kwargs: _Handler(  # noqa: E731
        *args, forced_content_type=forced_content_type, **kwargs
    )
    with socketserver.TCPServer((bind_host, 0), handler) as httpd:
        with open(port_file, "w", encoding="utf-8") as f:
            f.write(str(httpd.server_address[1]))
        httpd.serve_forever()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
