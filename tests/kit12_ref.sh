#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tests/kit12_ref/*.fun8 and tests/kit12_ref.txt: projects saved by wave 11 (the default 606 pattern, a HEX kit with locks,
# an edited 606* with locks, the 808 with locks, MY HEX 2, a 606* copy while the 808 plays) and wave 11's own render hashes
# of them. Builds tests/kit12_test.c with -DW11 against the firmware of commit REF (default 4fdd12b, the merge of wave 11)
# from git, not the tree.
#   sh tests/kit12_ref.sh [REF]
set -e
cd "$(dirname "$0")/.."
REF=${1:-4fdd12b}
D=$(mktemp -d)
git archive "$REF" firmware tests/hostsim.c tools assets | tar -x -C "$D"
(cd "$D" && python3 -c "import sys; sys.path.insert(0,'tools'); import build; build.generate()" >/dev/null)
cp tests/kit12_test.c "$D/tests/"
mkdir -p tests/kit12_ref
cc -O1 -w -DW11 -I"$D/build/gen" -I"$D/firmware/src" -I"$D/firmware/hal" -I"$D/tests" -o "$D/kit12_w11" "$D/tests/kit12_test.c" \
    build/host/libplx.a -lstdc++ -lm
"$D/kit12_w11" tests/kit12_ref | grep '^render' > tests/kit12_ref.txt
rm -rf "$D"
cat tests/kit12_ref.txt
