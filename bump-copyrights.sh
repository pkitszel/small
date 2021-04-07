#!/bin/bash
set -euo pipefail

git-touched-files(){
	{
		git diff --name-only --cached
		git diff --name-only "$branch"...HEAD
		git diff --name-only
	} | sort -u
}

branch=${1:-origin/master}
cd "$(git rev-parse --show-toplevel)" || exit

git-touched-files |
	xargs -r gawk -i inplace -v year="$(date +%Y)" '
		ENDFILE {
			if (!CopyrightPresent)
				print FILENAME " has no Copyright Present" > "/dev/stderr"
			CopyrightPresent = 0
		}
		/Copyright/ && $0 ~ year {
			print; CopyrightPresent = 1; next
		}
		/Copyright/ && /[^-][0-9]{4}[^-]/ {
			print gensub(/[0-9]{4}/, "\\0-" year, 1)
			TotChanges += CopyrightPresent = 1; next
		 }
		/Copyright/ && /[0-9]{4}-[0-9]{4}/ {
			print gensub(/([0-9]{4}-)[0-9]{4}/, "\\1" year, 1)
			TotChanges += CopyrightPresent = 1; next
		}
		1
		END {
			if (TotChanges)
				print TotChanges, "files updated" > "/dev/stderr"
		}'
