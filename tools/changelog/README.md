# Changelog generator

[`generate_changelog.py`](generate_changelog.py) produces the repository's
[`CHANGELOG.md`](../../CHANGELOG.md) from GitHub forensics. It was used to back-fill the
history and is the supported way to extend the changelog for new releases.

## What it does

For each `vMAJOR.MINOR.PATCH` tag range it merges two sources:

1. **Pull-request notes** — GitHub's `generate-notes` API (the same engine behind the
   "Generate release notes" button), which lists merged PRs with author and number.
2. **Direct commits** — `git log --no-merges` for the range, which recovers
   direct-to-`master` changes that never went through a PR. This matters: e.g. v1.5.4's
   PR notes list a single PR, but six more user-facing changes were pushed directly.

It then:

- **de-duplicates** commits that are merely a PR's squash (matched by `#NNN` and by a
  normalized title, including repairing GitHub's `…`-truncated PR titles from the squash
  commit or the PR body);
- **filters noise** — version bumps, typos, revert pairs, and CI/build infrastructure
  churn — using keyword rules that do **not** rely on commit-message prefixes, since
  dcmqi's history uses `ENH:`/`BUG:`/`COMP:` only sometimes;
- **categorizes** each entry into *Added / Changed / Fixed / Build & dependencies*
  (prefix when present, keyword heuristics otherwise);
- **attributes** every entry — PRs and commits alike — to a GitHub `@handle`, falling
  back to the git author name when a commit's email isn't linked to a GitHub account.

Entries describe what was *worked on* in each release range, not a guarantee of what
shipped; a few items may have been superseded or reverted within the same release.

## Requirements

- `git`
- [`gh`](https://cli.github.com/) CLI, authenticated (`gh auth status`)
- Python 3.8+

## Usage

```bash
# Regenerate the whole CHANGELOG.md (idempotent; safe to re-run):
python tools/changelog/generate_changelog.py

# Draft the section for an upcoming, not-yet-tagged release (changes since the
# latest tag) -- author this into CHANGELOG.md before tagging:
python tools/changelog/generate_changelog.py --next vX.Y.Z

# Regenerate one already-tagged release's section from git/PRs and print it:
python tools/changelog/generate_changelog.py --release v1.5.4

# Print a version's section straight from the committed CHANGELOG.md (no network);
# this is what CI uses to set the release body:
python tools/changelog/generate_changelog.py --extract v1.5.4
```

A first full run takes a couple of minutes (it resolves commit authors via the API).
Results are cached in `tools/changelog/.cache.json` (gitignored), so re-runs are fast.
`--extract` is a pure file read — no git, API, or cache — so it is safe and instant in CI.

## Release integration

`CHANGELOG.md` is the source of truth for release notes. The
[`release-notes.yml`](../../.github/workflows/release-notes.yml) workflow runs when a
`vX.Y.Z` release is published, calls `--extract`, and sets the release body via
`gh release edit`. The end-to-end release flow is in [`RELEASE.md`](../../RELEASE.md).

## Curated highlights

Auto-generated entries are exhaustive but flat. To surface the headline changes of a
release, add a short bullet list to the `HIGHLIGHTS` dict near the top of the script,
keyed by tag (see the `v1.3.0` and `v1.0.0` entries). They render as a `### Highlights`
block above the categorized lists.

## Tuning the noise filter

The `NOISE` list controls what is dropped. If a real change is being filtered out (too
aggressive) or churn is leaking through (too loose), adjust the patterns there and
regenerate. The categorization keywords live in `categorize()`.
