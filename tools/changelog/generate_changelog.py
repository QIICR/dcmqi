#!/usr/bin/env python3
"""Generate dcmqi's CHANGELOG.md from GitHub forensics.

Two sources are merged per release range:
  1. GitHub `generate-notes` API  -> PR-based "What's Changed" (clean backbone)
  2. `git log --no-merges`        -> direct-to-`master` commits the PR notes miss

Commits already represented by a PR are de-duplicated; noise (version bumps, typos,
reverts, CI/build infrastructure churn) is filtered; everything else is categorized
into Added / Changed / Fixed / Build. Conventional prefixes (ENH:/BUG:/COMP:/...) are
USED WHEN PRESENT but never assumed -- categorization and noise detection both fall back
to keyword heuristics on the bare subject, because dcmqi's history does not use prefixes
consistently.

Usage
-----
    # Regenerate the whole CHANGELOG.md (idempotent; run after tagging a release):
    python tools/changelog/generate_changelog.py

    # Print just one release's section, e.g. to paste into a GitHub release body:
    python tools/changelog/generate_changelog.py --release v1.5.4

Requirements: `git` and an authenticated `gh` CLI. See tools/changelog/README.md.
"""
import argparse
import json
import os
import re
import subprocess
import sys
from collections import OrderedDict
from datetime import date as _date

REPO = "QIICR/dcmqi"
HERE = os.path.dirname(os.path.abspath(__file__))
CACHE_PATH = os.path.join(HERE, ".cache.json")  # gitignored; speeds up re-runs
CHANGELOG_PATH = os.path.normpath(os.path.join(HERE, "..", "..", "CHANGELOG.md"))

HEADER = (
    "# Changelog\n\n"
    "All notable changes to **dcmqi** (DICOM for Quantitative Imaging) are documented here.\n\n"
    "Releases predating this file were **back-filled from GitHub forensics**: GitHub's\n"
    "pull-request release notes merged with direct-to-`master` commit messages, then\n"
    "de-duplicated, noise-filtered and categorized. Entries therefore describe what was\n"
    "worked on within each release range; a few items may have been superseded or reverted\n"
    "within the same release. New releases are appended with the same tool -- see\n"
    "`tools/changelog/`. The format follows "
    "[Keep a Changelog](https://keepachangelog.com/).\n"
)

def sh(args):
    return subprocess.run(args, capture_output=True, text=True).stdout

# A version tag supplied on the command line ultimately reaches `git`/`gh` argument
# lists (e.g. generate-notes' `tag_name=`), so validate it against the strict
# vMAJOR.MINOR.PATCH shape at the boundary -- anything else is rejected before it can
# flow into a subprocess invocation.
def normalize_tag(value):
    tag = value if value.startswith("v") else "v" + value
    if not re.fullmatch(r"v\d+\.\d+\.\d+", tag):
        sys.exit(f"invalid version {value!r}; expected vMAJOR.MINOR.PATCH (e.g. v1.5.5)")
    return tag

# ---------------------------------------------------------------- tags -------
def semver_tags():
    out = sh(["git", "for-each-ref", "--format=%(refname:short)\t%(creatordate:short)", "refs/tags"])
    tags = []
    for line in out.splitlines():
        if "\t" not in line:
            continue
        name, date = line.split("\t", 1)
        m = re.match(r"^v(\d+)\.(\d+)\.(\d+)$", name)
        if m:
            tags.append((name, date, tuple(int(x) for x in m.groups())))
    tags.sort(key=lambda t: t[2])  # ascending by semantic version
    return tags

# --------------------------------------------------------- normalization -----
PREFIX_RE = re.compile(
    r"^\s*(ENH|BUG|BUGFIX|FIX|COMP|COMPILE|CI|DOC|DOCS|WIP|STYLE|PERF|TEST|REF|REFACTOR|NF|API)\s*:\s*",
    re.I,
)

def strip_prefix(s):
    prev = None
    while prev != s:
        prev = s
        s = PREFIX_RE.sub("", s).strip()
    return s

def norm(s):
    s = strip_prefix(s)
    s = re.sub(r"\(#\d+\)\s*$", "", s)
    s = s.lower().strip().rstrip(".").strip()
    s = re.sub(r"\s+", " ", s)
    return s

def pretty(s):
    s = strip_prefix(s).strip()
    s = re.sub(r"\(#\d+\)\s*$", "", s)
    s = s.strip().rstrip("…").strip().rstrip(".")
    if s:
        s = s[0].upper() + s[1:]
    return s

# ----------------------------------------------------------- noise -----------
NOISE = [
    r"^\s*cmake\b.*version",
    r"set .*version to",
    r"update version to",
    r"bump .*version",
    r"^\s*v?\d+\.\d+\.\d+\s*$",
    r"\btypo\b",
    r"accident(al|ial)ly",
    r"^try(ing)? to fix",
    r"^another fix for",
    r"^revert\b",
    r"^merge\b",
    r"list workspace",
    r"add manual workflow run",
    r"trigger (ga|ci|build|workflow|actions)",
    r"^\s*ci\b",                       # CI-only direct pushes (meaningful CI work lands as PRs)
    r"^\s*(doc|docs)\b.*(readme|ack|reference|typo)",
    r"^\s*(upd|update)\s*$",
    r"^\s*wip\s*$",
    r"^\s*cleanups?\.?\s*$",
    r"^\s*initial plan\b",            # GitHub Copilot scaffolding commits
    # --- infrastructure churn, prefix-independent (subjects often lack a prefix) ---
    r"\bbadges?\b", r"\bshield\b",
    r"\bcircle\s?ci\b", r"\bappveyor\b", r"\btravis\b", r"\bdockcross\b",
    r"\.yml\b", r"\bworkflow\b", r"\bgithub action", r"\bdocker(file| image|_image)?\b",
    r"\bpip3?\b", r"\bxrange\b", r"\bunicode type\b", r"\blong type\b",
    r"resolve\b.*\bconflict", r"merge\s+conflict",
    r"missing newline", r"trailing whitespace", r"full folder name",
    r"\bmanual builds?\b", r"^\s*permit\b", r"^\s*also allow\b",
    r"^\s*trying\b", r"for debugging", r"split up .*build",
    r"better name for", r"^\s*use package with", r"^\s*use pip",
    r"removed? old ci", r"remove circle", r"upgrade circleci",
    r"grants? acks?", r"\backnowledge", r"update .*\backs?\b",
]
NOISE_RE = [re.compile(p, re.I) for p in NOISE]

def is_noise(subject):
    s = subject.strip()
    bare = strip_prefix(s)
    for rx in NOISE_RE:
        if rx.search(s) or rx.search(bare):
            return True
    return False

# ------------------------------------------------------- categorization ------
def categorize(subject):
    """Return one of: Fixed, Added, Build, Changed. Prefix wins if present, else
    keyword heuristics on the bare subject (prefixes are NOT assumed present)."""
    m = PREFIX_RE.match(subject)
    prefix = m.group(1).upper() if m else None
    bare = strip_prefix(subject)
    low = bare.lower()

    if prefix in ("BUG", "BUGFIX", "FIX"):
        return "Fixed"
    if prefix in ("COMP", "COMPILE"):
        return "Build"

    if re.search(r"\b(fix(e[ds])?|bug|crash|regression|resolve[sd]?|correct(ed|s)?|"
                 r"workaround|broken|segfault)\b", low):
        return "Fixed"
    if re.search(r"\b(itk|dcmtk|cmake|ninja|slicerexecutionmodel|externalproject|"
                 r"dependency|dependencies|build|compile|link(ing)?|toolchain|packaging)\b", low):
        return "Build"
    if prefix == "ENH" and re.search(r"\b(add|introduc|new|support|implement|enable)\b", low):
        return "Added"
    if re.match(r"^(add|added|new|introduce|introduced|implement|support|enable)\b", low):
        return "Added"
    return "Changed"

CAT_ORDER = ["Added", "Changed", "Fixed", "Build"]
CAT_TITLE = {"Added": "Added", "Changed": "Changed", "Fixed": "Fixed", "Build": "Build & dependencies"}

# ----------------------------------------------------- GitHub lookups --------
_commit_login_cache = {}
_pr_title_cache = {}

def load_cache():
    try:
        with open(CACHE_PATH) as f:
            d = json.load(f)
        _commit_login_cache.update(d.get("commit_login", {}))
        _pr_title_cache.update({int(k): v for k, v in d.get("pr_title", {}).items()})
    except Exception:
        pass

def save_cache():
    try:
        with open(CACHE_PATH, "w") as f:
            json.dump({"commit_login": _commit_login_cache,
                       "pr_title": {str(k): v for k, v in _pr_title_cache.items()}}, f)
    except Exception:
        pass

def fetch_commit_login(sha):
    """GitHub @login for a commit's author, or '' if it can't be mapped to an account."""
    if sha not in _commit_login_cache:
        login = ""
        try:
            d = json.loads(sh(["gh", "api", f"repos/{REPO}/commits/{sha}"]))
            login = ((d.get("author") or {}).get("login")) or ""
        except Exception:
            pass
        _commit_login_cache[sha] = login
    return _commit_login_cache[sha]

def fetch_pr_title(num):
    """Full PR title. GitHub stores long messages split as title + body joined by an
    ellipsis (title ends '…', body starts '…'); stitch them back if so."""
    if num not in _pr_title_cache:
        title = ""
        try:
            d = json.loads(sh(["gh", "api", f"repos/{REPO}/pulls/{num}"]))
            title = (d.get("title") or "").strip()
            body = (d.get("body") or "").strip()
            first = body.splitlines()[0].strip() if body else ""
            if title.endswith("…") and first.startswith("…"):
                title = title[:-1] + first[1:]
        except Exception:
            pass
        _pr_title_cache[num] = title
    return _pr_title_cache[num]

# "* TITLE by @user in URL/pull/NN" -- tolerate co-authors ("by @a with @b in ...").
# GitHub's generate-notes output uses single spaces; matching them literally (rather than
# \s+) keeps this regex linear -- variable-width \s+ around the co-author group could be
# distributed ambiguously and backtrack polynomially.
PR_LINE = re.compile(
    r"^\* (?P<title>.+?) by @(?P<user>[\w-]+)(?: with @[\w-]+)* in \S+/pull/(?P<num>\d+)")

def pr_entries(tag, prev, target=None):
    args = ["gh", "api", "-X", "POST", f"repos/{REPO}/releases/generate-notes",
            "-f", f"tag_name={tag}"]
    if prev:
        args += ["-f", f"previous_tag_name={prev}"]
    if target:  # for a not-yet-tagged release: generate notes against this commit
        args += ["-f", f"target_commitish={target}"]
    out = sh(args)
    entries = []
    try:
        body = json.loads(out)["body"]
    except Exception:
        return entries
    for line in body.splitlines():
        m = PR_LINE.match(line.strip())
        if m:
            entries.append({"title": m.group("title"), "user": m.group("user"),
                            "num": int(m.group("num")), "source": "pr"})
    return entries

def commit_entries(end, prev):
    rng = f"{prev}..{end}" if prev else end
    out = sh(["git", "log", "--no-merges", "--pretty=%h%x09%an%x09%s", rng])
    rows = []
    for line in out.splitlines():
        parts = line.split("\t", 2)
        if len(parts) == 3:
            rows.append(tuple(parts))  # (hash, author_name, subject)

    reverted = set()
    for _, _, s in rows:
        m = re.match(r'^\s*Revert\s+"(.*)"\s*$', s.strip())
        if m:
            reverted.add(norm(m.group(1)))

    entries = []
    for h, an, s in rows:
        if is_noise(s) or norm(s) in reverted:
            continue
        m = re.search(r"\(#(\d+)\)\s*$", s)
        entries.append({"title": s, "hash": h, "author": an,
                        "num": int(m.group(1)) if m else None, "source": "commit"})
    return entries

# --------------------------------------------------------- highlights --------
# Hand-curated highlights to surface at the top of a release section. Add an entry
# here when cutting a release whose headline changes deserve emphasis.
HIGHLIGHTS = {
    "v1.3.0": [
        "Upgrade to ITK v5",
        "Switch to GitHub Actions from platform-specific CI solutions",
        "Major performance improvements for DICOM SEG conversion",
        "Added ability to save non-overlapping DICOM SEG segments into a single file",
    ],
    "v1.0.0": [
        "First official release of dcmqi, accompanying the article submitted to the "
        "ITCR special issue of *Cancer Research*",
        "Shipped with documentation and introductory tutorials",
    ],
}

# --------------------------------------------------------- assemble ----------
def build_section(tag, date, prev, end_ref=None, target=None):
    """Build one release section. `end_ref` is the git range end for commit enrichment
    (defaults to `tag`; pass "HEAD" for an unreleased draft); `target` is the
    generate-notes target_commitish, needed when `tag` is not yet a real tag."""
    end_ref = end_ref or tag
    prs = pr_entries(tag, prev, target)
    # The first release has no previous tag; "prev..tag" would be the entire
    # pre-1.0 history. Don't enrich it with commits -- rely on PR notes + highlights.
    commits = commit_entries(end_ref, prev) if prev else []

    pr_nums = {p["num"] for p in prs}
    # GitHub truncates long PR titles with an ellipsis; track a (possibly partial)
    # key per PR so a squash commit carrying the full message still de-duplicates.
    for p in prs:
        n = norm(p["title"])
        p["_trunc"] = n.endswith("…")
        p["_key"] = n.rstrip("…").strip()
    pr_exact = {p["_key"] for p in prs if not p["_trunc"]}
    pr_trunc = [p for p in prs if p["_trunc"] and len(p["_key"]) >= 12]

    merged = list(prs)
    dropped_dup = 0
    for c in commits:
        cn = norm(c["title"])
        if c["num"] in pr_nums or cn in pr_exact:
            dropped_dup += 1
            continue
        hit = next((p for p in pr_trunc if cn.startswith(p["_key"])), None)
        if hit:
            hit["title"] = pretty(c["title"])  # repair truncation with full commit text
            hit["_trunc"] = False
            dropped_dup += 1
            continue
        merged.append(c)

    # any PR title still truncated (no squash commit to repair from): fetch full text
    for p in merged:
        if p["source"] == "pr" and "…" in p["title"]:
            p["title"] = fetch_pr_title(p["num"]) or p["title"].replace("…", "")

    buckets = OrderedDict((c, []) for c in CAT_ORDER)
    seen = set()
    for e in merged:
        key = norm(e["title"])
        if key in seen:
            continue
        seen.add(key)
        text = pretty(e["title"])
        if not text:
            continue
        cat = categorize(e["title"])
        if e["source"] == "pr":
            ref = f"[#{e['num']}](https://github.com/{REPO}/pull/{e['num']}), @{e['user']}"
        else:
            login = fetch_commit_login(e["hash"])
            who = f"@{login}" if login else e["author"]  # fall back to git name if unmapped
            ref = f"[`{e['hash']}`](https://github.com/{REPO}/commit/{e['hash']}), {who}"
        buckets[cat].append(f"- {text} ({ref})")

    lines = [f"## [{tag.lstrip('v')}]"
             f"(https://github.com/{REPO}/releases/tag/{tag}) - {date}"]
    if tag in HIGHLIGHTS:
        lines += ["", "### Highlights"] + [f"- {h}" for h in HIGHLIGHTS[tag]]
    for cat in CAT_ORDER:
        if buckets[cat]:
            lines += ["", f"### {CAT_TITLE[cat]}"] + buckets[cat]
    if prev:
        lines += ["", f"_[Full set of changes](https://github.com/{REPO}/compare/{prev}...{tag})_"]
    return "\n".join(lines), len(prs), len(merged) - len(prs), dropped_dup

# --------------------------------------------------------- extract ----------
def extract_section(version):
    """Return VERSION's section body from the committed CHANGELOG.md (no network).
    The `## [version] - date` header line is dropped (a release page already shows the
    version as its title). Used by CI to set the release body."""
    label = version.lstrip("v")
    try:
        with open(CHANGELOG_PATH) as f:
            lines = f.read().splitlines()
    except OSError:
        return ""
    out, capturing = [], False
    for line in lines:
        if line.startswith("## ["):
            if capturing:
                break
            capturing = line.startswith(f"## [{label}]")
            continue  # drop the version header line itself
        if capturing:
            out.append(line)
    return "\n".join(out).strip()

# ------------------------------------------------------------- main ----------
def main():
    ap = argparse.ArgumentParser(description="Generate dcmqi CHANGELOG.md from GitHub forensics.")
    g = ap.add_mutually_exclusive_group()
    g.add_argument("--release", metavar="TAG",
                   help="regenerate one existing release's section from git/PRs and print it")
    g.add_argument("--next", dest="next_version", metavar="VERSION",
                   help="draft the section for an upcoming, not-yet-tagged release (changes "
                        "since the latest tag) and print it -- author it before tagging")
    g.add_argument("--extract", metavar="VERSION",
                   help="print VERSION's section from the existing CHANGELOG.md (no network); "
                        "used by CI to set the release body")
    args = ap.parse_args()

    # --extract is a pure file read: no git, no API, no cache.
    if args.extract:
        section = extract_section(normalize_tag(args.extract))
        if not section:
            sys.exit(f"no CHANGELOG.md section found for {args.extract}")
        print(section)
        return

    load_cache()
    try:
        if args.next_version:
            vtag = normalize_tag(args.next_version)
            tags = semver_tags()
            prev = tags[-1][0] if tags else None
            head = sh(["git", "rev-parse", "HEAD"]).strip()
            section, *_ = build_section(vtag, _date.today().isoformat(), prev,
                                        end_ref="HEAD", target=head)
            print(section)
            return

        tags = semver_tags()
        if not tags:
            sys.exit("no vMAJOR.MINOR.PATCH tags found")

        if args.release:
            want = normalize_tag(args.release)
            idx = next((i for i, t in enumerate(tags) if t[0] == want), None)
            if idx is None:
                sys.exit(f"tag {want} not found among semver tags")
            name, date, _ = tags[idx]
            prev = tags[idx - 1][0] if idx > 0 else None
            section, *_ = build_section(name, date, prev)
            print(section)
            return

        sections = []
        print("tag       PRs  +commits  (dups dropped)", file=sys.stderr)
        for i in range(len(tags) - 1, -1, -1):
            name, date, _ = tags[i]
            prev = tags[i - 1][0] if i > 0 else None
            section, npr, ncom, ndup = build_section(name, date, prev)
            sections.append(section)
            print(f"{name:9} {npr:3}  {ncom:6}    ({ndup} dup)", file=sys.stderr)
        with open(CHANGELOG_PATH, "w") as f:
            f.write(HEADER + "\n\n" + "\n\n".join(sections).rstrip() + "\n")
        print(f"\nWrote {CHANGELOG_PATH}", file=sys.stderr)
    finally:
        save_cache()

if __name__ == "__main__":
    main()
