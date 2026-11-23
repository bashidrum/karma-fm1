#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tests/mx11_ref.txt: wave 10's own render hashes of the matrices tests/mx11_test.c plays (layout 1, as waves 5 to 10 stored
# them). Builds the test against the firmware of commit REF (default ec814ba, the merge of wave 10) from git, not the tree.
#   sh tests/mx11_ref.sh [REF]
set -e
cd "$(dirname "$0")/.."
REF=${1:-ec814ba}
D=$(mktemp -d)
git archive "$REF" firmware tests/hostsim.c tools assets | tar -x -C "$D"
(cd "$D" && python3 -c "import sys; sys.path.insert(0,'tools'); import build; build.generate()" >/dev/null)
cp tests/mx11_test.c "$D/tests/"
cc -O1 -w -I"$D/build/gen" -I"$D/firmware/src" -I"$D/firmware/hal" -I"$D/tests" -o "$D/mx11_w10" "$D/tests/mx11_test.c" \
    build/host/libplx.a -lstdc++ -lm
"$D/mx11_w10" | grep '^render' > tests/mx11_ref.txt
rm -rf "$D"
cat tests/mx11_ref.txt
