#!/bin/bash
set -euo pipefail

# usage:
#   there is only one parameter, base branch/tag/commit to compare against, eg:
# ./check-kdoc.sh  linus/master

command -V gawk >/dev/null
cwd="$(pwd)"
repo_home="$(git rev-parse --show-toplevel)"
cd "$repo_home"
local_prefix="${cwd/"$repo_home/"}"

function warn() {
	echo "$@" >&2
}

function die() {
	rc=$?
	warn "$@"
	[ $rc = 0 ] && rc=99
	exit $rc
}

function ensure_clean_git_state() {
	git diff --exit-code >/dev/null
}

function kdoc() {
	local files=()
	for f in "$@"; do
		test -s "$f" && files+=("$f")
	done
	"$repo_home"/scripts/kernel-doc --none -Wall "${files[@]}" 2>&1
}

function strip-out-linenums() {
	gawk '{ print gensub(/:[0-9]+:/, ":", 1) }'
}

# run $1 with the rest of params but ignore errors
function ok() {
	tool="$1"
	shift
	"$tool" "$@" || true
}

function main() {
	base_hash="$1"
	baseline_kdoc_out=$(mktemp)
	current_kdoc_out=$(mktemp)
	grep_pat_file=$(mktemp)
	trap 'rm "$baseline_kdoc_out" "$current_kdoc_out" "$grep_pat_file"' EXIT
	
	files=$(git diff --name-only "$base_hash")
	if [ -z "$files" ]; then
		warn 'no files modified since' "$base_hash"
		return 0
	fi

	git checkout -q "$base_hash"
	kdoc $files >"$baseline_kdoc_out"

	git checkout -q -
	kdoc $files >"$current_kdoc_out"

	ok diff -u0	<(strip-out-linenums < "$baseline_kdoc_out") \
			<(strip-out-linenums < "$current_kdoc_out") \
	| awk '/^[+][^+]/ && sub(/+/, "") { $1 = ""; print }' \
	> "$grep_pat_file"

	ok grep -f "$grep_pat_file" "$current_kdoc_out" \
	| awk -v lp="$local_prefix/" '
		sub(/warning: /, "") {
			if (index($0, lp) == 1)
				$0 = substr($0, length(lp) + 1)
		}
		++cnt
		END { exit cnt }
	'
}

ensure_clean_git_state || die 'git must be in a clean state'
main "$@"
