#!/usr/bin/env bash
#
# check-format.sh — run the CI clang-format style check locally.
#
# Mirrors .github/workflows/style.yml exactly: clang-format 19 over so3/so3
# (excluding *.S) and so3/usr (excluding the vendored / generated trees), on
# git-TRACKED files only — just like the CI's actions/checkout (so generated
# build artefacts such as the kconfig parser are naturally ignored).
#
# Usage:
#   scripts/check-format.sh           # report files that need formatting (exit 1 if any)
#   scripts/check-format.sh --fix     # reformat the offending files in place
#
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

FIX=0
[ "${1:-}" = "--fix" ] && FIX=1

# Prefer clang-format-19 (the version the CI pins); fall back with a warning.
CF=""
for c in clang-format-19 clang-format; do
	command -v "$c" >/dev/null 2>&1 && { CF="$c"; break; }
done
[ -n "$CF" ] || { echo "error: clang-format not found (install clang-format-19)" >&2; exit 2; }
ver="$("$CF" --version | grep -oE '[0-9]+' | head -1)"
[ "$ver" = "19" ] || echo "warning: using $CF (v$ver); the CI uses clang-format 19 — results may differ" >&2

# C/C++/Protobuf extensions — the jidicula/clang-format-action default set.
ext_re='\.(c|h|C|H|cpp|hpp|cc|hh|cxx|hxx|cu|cuh|proto)$'

fail=0

# check_path <dir> <exclude-regex>   (the two style.yml matrix entries)
check_path() {
	local path="$1" exclude="$2" f
	while IFS= read -r f; do
		[[ "$f" =~ $ext_re ]] || continue
		[ -n "$exclude" ] && [[ "$f" =~ $exclude ]] && continue
		if [ "$FIX" -eq 1 ]; then
			"$CF" -i "$f"
		elif ! "$CF" --dry-run --Werror "$f" >/dev/null 2>&1; then
			echo "FAIL: $f"
			fail=1
		fi
	done < <(git ls-files "$path")
}

check_path "so3/so3" '\.S$'
check_path "so3/usr" '(micropython|libxml2|usr/lib/linux|lvgl|lv_)'

if [ "$FIX" -eq 1 ]; then
	echo "clang-format: reformatted in place ($CF)"
elif [ "$fail" -eq 0 ]; then
	echo "clang-format: all clean ✓ ($CF)"
else
	echo
	echo "clang-format violations above. Fix with:  scripts/check-format.sh --fix"
	exit 1
fi
