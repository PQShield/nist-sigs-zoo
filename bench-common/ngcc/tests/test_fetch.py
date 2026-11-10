import hashlib
import http.server
import threading

import pytest

from fetch import ArchiveError, fetch_one

ZIP = b"PK\x03\x04archive"


def row_for(path):
    return {
        "id": "sign-01",
        "kind": "sig",
        "name": "X",
        "zip": "x.zip",
        "url": path.as_uri(),
        "page": "",
        "bytes": "",
        "sha256": "",
    }


def test_record_pins_size_and_digest(tmp_path):
    src = tmp_path / "x.zip"
    src.write_bytes(ZIP)
    row, cache = row_for(src), tmp_path / "cache"
    assert fetch_one(row, cache, record=True) == "recorded"
    assert row["bytes"] == str(len(ZIP))
    assert row["sha256"] == hashlib.sha256(ZIP).hexdigest()
    assert fetch_one(row, cache, record=False) == "cached"


def test_unpinned_archive_needs_record(tmp_path):
    src = tmp_path / "x.zip"
    src.write_bytes(ZIP)
    with pytest.raises(ArchiveError, match="--record"):
        fetch_one(row_for(src), tmp_path / "cache", record=False)


def test_changed_archive_is_refused_and_moved_aside(tmp_path):
    src = tmp_path / "x.zip"
    src.write_bytes(ZIP)
    row, cache = row_for(src), tmp_path / "cache"
    fetch_one(row, cache, record=True)
    (cache / "sign-01.zip").write_bytes(b"PK\x03\x04tampered")
    with pytest.raises(ArchiveError, match="differs"):
        fetch_one(row, cache, record=True)  # --record never re-pins a pinned archive
    assert not (cache / "sign-01.zip").exists()
    assert list(cache.glob("sign-01.zip.mismatch-*"))


def serve(body, claimed=None, name="x.zip"):
    class H(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            self.send_response(200)
            self.send_header(
                "Content-Length", str(len(body) if claimed is None else claimed)
            )
            self.end_headers()
            self.wfile.write(body)

        def log_message(self, *a):
            pass

    srv = http.server.HTTPServer(("127.0.0.1", 0), H)
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return srv, f"http://127.0.0.1:{srv.server_port}/{name}"


def test_truncated_download_is_rejected(tmp_path):
    srv, url = serve(ZIP, claimed=len(ZIP) + 100)
    row = {**row_for(tmp_path / "x"), "url": url}
    try:
        with pytest.raises(ArchiveError, match="truncated"):
            fetch_one(row, tmp_path / "cache", record=True)
    finally:
        srv.shutdown()
    assert not list((tmp_path / "cache").glob("sign-01*"))


@pytest.mark.parametrize("body", [b"", b"<html>nope</html>"])
def test_empty_or_non_archive_is_rejected(tmp_path, body):
    src = tmp_path / "x.zip"
    src.write_bytes(body)
    with pytest.raises(ArchiveError):
        fetch_one(row_for(src), tmp_path / "cache", record=True)
    assert not list((tmp_path / "cache").glob("sign-01*"))


def test_record_refuses_cache_that_differs_from_fresh(tmp_path):
    src = tmp_path / "x.zip"
    src.write_bytes(ZIP)
    cache = tmp_path / "cache"
    cache.mkdir()
    (cache / "sign-01.zip").write_bytes(b"PK\x03\x04stale")
    row = row_for(src)
    with pytest.raises(ArchiveError, match="fresh"):
        fetch_one(row, cache, record=True)
    assert row["sha256"] == ""
