#!/bin/bash
set -Eeuo pipefail

# This preamble will embed $0 and $@ into "shell archive", print it, then exit.
# Paths are not recreated at extraction (--transform in tar call).
# Resulting archive would call $0 with $@ args.
# $0 should be defined within this script, (embedding preamble would not be embedded).
MAIN_SCRIPT="$(mktemp -d)/main.sh"
awk 'NR>1&&/^#!/,0' "$0" > "${MAIN_SCRIPT}"
chmod +x "${MAIN_SCRIPT}"
echo	\
	'set -Eeuo pipefail;' \
	"EMBEDDED_SCRIPT='$(tar --transform='s/.*\///' -c "${MAIN_SCRIPT}" "$@" | gzip -9 | base64)';" \
	'WORKDIR="$(mktemp -d)"; cd "${WORKDIR}";' \
	'base64 -d <<< "${EMBEDDED_SCRIPT}" | gunzip | tar -x;' \
	./"$(basename "${MAIN_SCRIPT}");" \
	'cd -; rm -rf "${WORKDIR}";'
rm -rf "$(dirname "${MAIN_SCRIPT}")"
exit # end of preamble, actual script below (from shebang)
