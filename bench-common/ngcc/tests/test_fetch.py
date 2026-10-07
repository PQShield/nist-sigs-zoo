import hashlib

import pytest
from fetch import ArchiveError, fetch_one


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
    src.write_bytes(b"archive")
    row, cache = row_for(src), tmp_path / "cache"
    assert fetch_one(row, cache, record=True) == "recorded"
    assert row["bytes"] == "7"
    assert row["sha256"] == hashlib.sha256(b"archive").hexdigest()
    assert fetch_one(row, cache, record=False) == "cached"


def test_unpinned_archive_needs_record(tmp_path):
    src = tmp_path / "x.zip"
    src.write_bytes(b"archive")
    with pytest.raises(ArchiveError, match="--record"):
        fetch_one(row_for(src), tmp_path / "cache", record=False)


def test_changed_archive_is_refused_and_moved_aside(tmp_path):
    src = tmp_path / "x.zip"
    src.write_bytes(b"archive")
    row, cache = row_for(src), tmp_path / "cache"
    fetch_one(row, cache, record=True)
    (cache / "sign-01.zip").write_bytes(b"tampered")
    with pytest.raises(ArchiveError, match="differs"):
        fetch_one(row, cache, record=True)  # --record never re-pins a pinned archive
    assert not (cache / "sign-01.zip").exists()
    assert (cache / "sign-01.zip.mismatch").exists()
