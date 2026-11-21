#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The offline backup page (web/make_site.py offline_editor, web/editor.html): one file that needs nothing next to it."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "web"))
import make_site  # noqa: E402

src = (ROOT / "web" / "editor.html").read_text(encoding="utf-8")
off = make_site.offline_editor(src)
bad = 0


def check(what, ok):
    global bad
    print(f"site: {what:<70} {'ok' if ok else 'FAIL'}")
    bad += not ok


check("the backup page links the offline file (download)", src.count('download="karma-editor.html"') == 1)
check("the installer page links it too",
      (ROOT / "web" / "index_pkg.html").read_text(encoding="utf-8").count('href="../editor/karma-editor.html"') == 1)
check("offline: no icon font (the editor's Fukiai font went with the editing)", "fukiai" not in off.lower())
check("offline: no download link to itself", "<!--DL-->" not in off and 'id="offline"' not in off)
check("offline: the installer online",
      'href="../installer/"' not in off and make_site.ONLINE + "webapp/installer/" in off)
rel = [m for m in re.findall(r'(?:src|href)="([^"#]+)"', off) if not re.match(r"(https?:|data:|mailto:)", m)]
check("offline: no other file next to it (no relative src / href)", not rel)
check("offline: no url() to a file", not re.search(r'url\((?!data:)["\']?[A-Za-z]', off))
print("site:", "FAILED" if bad else "offline backup page test passed")
sys.exit(1 if bad else 0)
