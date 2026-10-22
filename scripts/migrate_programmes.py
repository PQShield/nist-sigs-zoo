#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# dependencies = ["ruamel.yaml>=0.18"]
# ///
"""One-off migration: replace `tags: [round-N]` with `programmes:` maps.

Signatures: versions[].tags [round-N] -> versions[].programmes {nist-onramp: round-N}.
NIST standards and KpqC picks get their memberships from SIG_EXTRA.
KEMs: a scheme-level `programmes:` map from KEM_PROGRAMMES.

Usage:
    uv run scripts/migrate_programmes.py
"""

import sys
from pathlib import Path

from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap, CommentedSeq

REPO_ROOT = Path(__file__).parent.parent
SCHEMES_DIR = REPO_ROOT / "data" / "schemes"
KEMS_DIR = REPO_ROOT / "data" / "kems"

# Scheme name -> memberships added to *every* version of that scheme.
SIG_EXTRA_ALL_VERSIONS: dict[str, dict[str, str]] = {
    "ML-DSA": {"nist-pqc": "standardised"},
    "SLH-DSA": {"nist-pqc": "standardised"},
    "Falcon": {"nist-pqc": "selected"},
}
# Scheme name -> memberships added to the *newest* version only.
SIG_EXTRA_LATEST: dict[str, dict[str, str]] = {
    "HAETAE": {"kpqc": "selected"},
    "AIMer": {"kpqc": "selected"},
}
KEM_PROGRAMMES: dict[str, dict[str, str]] = {
    "ML-KEM": {"nist-pqc": "standardised"},
    "HQC": {"nist-pqc": "selected"},
    "FrodoKEM": {"nist-pqc": "round-3", "iso": "in-progress"},
    "Classic McEliece": {"nist-pqc": "round-4", "iso": "in-progress"},
    "NTRU": {"nist-pqc": "round-3"},
    "NTRU Prime": {"nist-pqc": "round-3"},
    "SABER": {"nist-pqc": "round-3"},
    "SMAUG-T": {"kpqc": "selected"},
    "NTRU+": {"kpqc": "selected"},
}


def make_yaml() -> YAML:
    yaml = YAML()
    yaml.preserve_quotes = True
    yaml.width = 4096
    yaml.indent(mapping=2, sequence=2, offset=0)
    return yaml


def set_programmes(version: CommentedMap, extra: dict[str, str]) -> None:
    programmes = CommentedMap(version.get("programmes") or {})
    rounds = []
    for tag in version.get("tags") or []:
        if not str(tag).startswith("round-"):
            sys.exit(f"unexpected tag {tag!r} in version {version.get('version')!r}")
        rounds.append(str(tag))
    if len(rounds) == 1:
        programmes["nist-onramp"] = rounds[0]
    elif rounds:
        # A version tagged with several rounds keeps all of them as a flow list.
        stages = CommentedSeq(rounds)
        stages.fa.set_flow_style()
        programmes["nist-onramp"] = stages
    programmes.update(extra)
    if "tags" in version:
        del version["tags"]
    if programmes:
        # Place programmes right after status, where tags used to sit.
        keys = list(version.keys())
        version.insert(keys.index("status") + 1, "programmes", programmes)


def migrate_signatures(yaml: YAML) -> None:
    for path in sorted(SCHEMES_DIR.glob("*.yaml")):
        doc = yaml.load(path)
        name = doc["name"]
        newest = max(doc["versions"], key=lambda v: str(v["date"]))
        for version in doc["versions"]:
            extra = dict(SIG_EXTRA_ALL_VERSIONS.get(name, {}))
            if version is newest:
                extra.update(SIG_EXTRA_LATEST.get(name, {}))
            set_programmes(version, extra)
        yaml.dump(doc, path)


def migrate_kems(yaml: YAML) -> None:
    for path in sorted(KEMS_DIR.glob("*.yaml")):
        doc = yaml.load(path)
        memberships = KEM_PROGRAMMES.get(doc["name"])
        if memberships:
            keys = list(doc.keys())
            doc.insert(
                keys.index("status") + 1, "programmes", CommentedMap(memberships)
            )
        yaml.dump(doc, path)


def main() -> None:
    yaml = make_yaml()
    migrate_signatures(yaml)
    migrate_kems(yaml)


if __name__ == "__main__":
    main()
