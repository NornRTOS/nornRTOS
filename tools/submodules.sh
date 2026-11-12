#!/bin/bash
# Registers the module checkouts under modules/ as git submodules with the URLs of
# .gitmodules; modules already registered are skipped, so it can be rerun when a module is
# added. Run after the module repository has its first commit pushed to the
# organisation (git submodule add needs a reachable URL and a committed checkout); the
# existing directories are kept. Afterwards a fresh clone is
#   git clone --recurse-submodules git@github.com:NornRTOS/nornRTOS.git
set -eu
cd "$(dirname "$0")/.."
for m in nornCore nornNET nornWIFI nornHTTP nornSHELL nornStore; do
    url=$(git config -f .gitmodules submodule.modules/$m.url)
    if git ls-files --stage modules/$m | grep -q '^160000 '; then
        echo "modules/$m already registered"
    elif git -C modules/$m rev-parse HEAD > /dev/null 2>&1; then
        git submodule add "$url" modules/$m
        echo "modules/$m registered ($url)"
    else
        echo "modules/$m has no commits yet: commit and push it first"; exit 1
    fi
done
