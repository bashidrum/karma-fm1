#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tests/dfx_ref/*.fun8 and *.raw: projects saved by wave 14 with the kit-wide drum sends (FX > SENDS), the sounds' own and send
# locks, and wave 14's own audio of two bars of each. Builds tests/dfx_test.c with -DW14 against the firmware of commit REF
# (default ben's 0212369's wave 14 tree, before the drum effects moved under FX), not the tree.
#   sh tests/dfx_ref.sh [REF]
set -e
cd "$(dirname "$0")/.."
REF=${1:-c73413f}
D=$(mktemp -d)
git archive "$REF" firmware tests/hostsim.c tools assets | tar -x -C "$D"
(cd "$D" && python3 -c "import sys; sys.path.insert(0,'tools'); import build; build.generate()" >/dev/null)
cp tests/dfx_test.c "$D/tests/"
mkdir -p tests/dfx_ref
cc -O1 -w -DW14 -I"$D/build/gen" -I"$D/firmware/src" -I"$D/firmware/hal" -I"$D/tests" -o "$D/dfx_w14" "$D/tests/dfx_test.c" \
    build/host/libplx.a -lstdc++ -lm
"$D/dfx_w14" tests/dfx_ref
rm -rf "$D"
