#!/bin/bash
# Snapshot release of the norn repositories: the public main of each repository is one
# commit, the tree of the private dev branch, without history; its message is the
# release's own. Run from anywhere in the nornRTOS tree:
#
#   tools/release.sh [-n] [-m BODY] REPO VERSION [REPO VERSION ...]
#
#   REPO      nornCore nornNET nornWIFI nornHTTP nornSHELL nornStore nornRTOS nornDocs,
#             or all (every one of them at the same VERSION)
#   VERSION   the tag, vX.Y.Z
#   -n        dry run: every check and the trees, no commit, tag or push
#   -m BODY   body of the release commit message, after the subject "REPO VERSION";
#             the same body for every repository of the run
#
# Per repository, modules first, nornDocs, nornRTOS last:
#   1. the working tree is clean and dev is pushed (dev == private/dev after a fetch);
#   2. the tree of dev becomes a parentless commit with git commit-tree; for nornRTOS the
#      tree is built in a temporary index with every submodule entry replaced by the
#      module's public main commit, which must carry the tree the dev entry has (release
#      the module in the same run, or earlier);
#   3. the commit is force pushed to public main, tagged VERSION (annotated; the tag must
#      not exist on public yet) and the tag pushed; the local main follows.
# Nothing of dev's history or its commit ids leaves the machine. The old public commits
# stay reachable through older tags only; the script lists them.
#
# Remotes: private (git@github.com:TenforwardAB/<name>) holds dev, public
# (git@github.com:NornRTOS/<name>) holds main. NORNDOCS=<path> if nornDocs is not next
# to nornRTOS.
set -u
cd "$(dirname "$0")/.."
ROOT=$PWD
NORNDOCS=${NORNDOCS:-$ROOT/../nornDocs}
MODULES="nornCore nornNET nornWIFI nornHTTP nornSHELL nornStore"
ORDER="$MODULES nornDocs nornRTOS"

DRY=0; BODY=
while getopts "nm:" o; do
    case $o in
        n) DRY=1 ;;
        m) BODY=$OPTARG ;;
        *) exit 2 ;;
    esac
done
shift $((OPTIND - 1))
[ $# -ge 2 ] && [ $(($# % 2)) -eq 0 ] || { sed -n '2,/^set -u/p' "$0" | sed '$d'; exit 2; }

die()  { echo "release: $*" >&2; exit 1; }
say()  { echo "$([ $DRY = 1 ] && echo '[dry] ')$*"; }

declare -A WANT          # repo -> version
while [ $# -gt 0 ]; do
    if [ "$1" = all ]; then
        for r in $ORDER; do WANT[$r]=$2; done
    else
        case " $ORDER " in *" $1 "*) WANT[$1]=$2 ;; *) die "unknown repository $1" ;; esac
    fi
    shift 2
done
for r in "${!WANT[@]}"; do
    [[ ${WANT[$r]} =~ ^v[0-9]+\.[0-9]+\.[0-9]+([-.][0-9A-Za-z.]+)?$ ]] || die "$r: version ${WANT[$r]} is not vX.Y.Z"
done

repo_dir() {
    case $1 in
        nornRTOS) echo "$ROOT" ;;
        nornDocs) echo "$NORNDOCS" ;;
        *)        echo "$ROOT/modules/$1" ;;
    esac
}

# The checks every repository passes before anything is written.
check() {   # repo
    local d; d=$(repo_dir "$1")
    [ -d "$d/.git" ] || [ -f "$d/.git" ] || die "$1: no repository at $d"
    git -C "$d" remote get-url public  > /dev/null 2>&1 || die "$1: no remote 'public'"
    git -C "$d" remote get-url private > /dev/null 2>&1 || die "$1: no remote 'private'"
    [ -z "$(git -C "$d" status --porcelain --untracked-files=no)" ] || die "$1: uncommitted changes; commit them on dev first"
    local untracked; untracked=$(git -C "$d" status --porcelain | grep -c '^??')
    [ "$untracked" = 0 ] || echo "$1: note, $untracked untracked file(s) are not part of the tree"
    git -C "$d" fetch -q private && git -C "$d" fetch -q --tags public || die "$1: fetch failed"
    git -C "$d" rev-parse -q --verify dev > /dev/null || die "$1: no branch dev"
    [ "$(git -C "$d" rev-parse dev)" = "$(git -C "$d" rev-parse private/dev)" ] || die "$1: dev is not what private/dev has; push dev first"
    [ -z "$(git -C "$d" ls-remote --tags public "refs/tags/${WANT[$1]}")" ] || die "$1: tag ${WANT[$1]} already exists on public"
}

# The tree the release commit gets: dev's, with the submodules of nornRTOS pointed at
# their public main commits (set by this run or an earlier one).
release_tree() {   # repo
    local d; d=$(repo_dir "$1")
    if [ "$1" != nornRTOS ]; then
        git -C "$d" rev-parse "dev^{tree}"; return
    fi
    local idx; idx=$(mktemp)
    (
        cd "$d" && export GIT_INDEX_FILE=$idx && git read-tree dev || exit 1
        for m in $MODULES; do
            local p=modules/$m devlink pub
            devlink=$(git rev-parse "dev:$p") || exit 1
            if [ $DRY = 1 ] && [ -n "${WANT[$m]+x}" ]; then
                pub=$devlink        # this run releases it; the real commit does not exist yet
            else
                pub=$(git -C "$p" rev-parse -q --verify public/main) || { echo "nornRTOS: $m has no public main; release it first" >&2; exit 1; }
                [ "$(git -C "$p" rev-parse "$devlink^{tree}")" = "$(git -C "$p" rev-parse "$pub^{tree}")" ] \
                    || { echo "nornRTOS: $m on public main is not the tree dev points at (${devlink:0:7}); release $m in this run" >&2; exit 1; }
            fi
            git update-index --add --cacheinfo "160000,$pub,$p" || exit 1
        done
        git write-tree
    )
    local rc=$?; rm -f "$idx"; return $rc
}

release() {   # repo
    local r=$1 v=${WANT[$1]} d tree c
    d=$(repo_dir "$r")
    tree=$(release_tree "$r") || exit 1
    local cur; cur=$(git -C "$d" rev-parse -q --verify "public/main^{tree}" 2>/dev/null || true)
    local same=0
    [ "$tree" = "$cur" ] && [ "$(git -C "$d" rev-list --count public/main)" = 1 ] && same=1
    local msg="$r $v"; [ -n "$BODY" ] && msg="$msg

$BODY"
    if [ $DRY = 1 ]; then
        if [ $same = 1 ]; then say "$r: public main already is this tree, would only tag it $v"
        else say "$r: would commit tree ${tree:0:7} as \"$r $v\", force push it to public main, tag $v"; fi
        return
    fi
    if [ $same = 1 ]; then
        c=$(git -C "$d" rev-parse public/main)
    else
        c=$(git -C "$d" commit-tree "$tree" -m "$msg") || die "$r: commit-tree failed"
        git -C "$d" push -q -f public "$c:refs/heads/main" || die "$r: push to public main failed"
    fi
    git -C "$d" tag -a "$v" "$c" -m "$r $v" || die "$r: tag failed"
    git -C "$d" push -q public "refs/tags/$v" || die "$r: push of tag $v failed"
    git -C "$d" branch -f main "$c" > /dev/null 2>&1 || true
    say "$r: public main = ${c:0:7} ($v)"
    local old; old=$(git -C "$d" ls-remote --tags public | grep -v '\^{}' | awk '{print $2}' | sed 's#refs/tags/##' | grep -vx "$v" | tr '\n' ' ')
    [ -n "$old" ] && echo "$r: older public tags keep their commits reachable: $old (git -C $d push public --delete <tag> removes one)"
}

for r in $ORDER; do [ -n "${WANT[$r]+x}" ] && check "$r"; done
for r in $ORDER; do [ -n "${WANT[$r]+x}" ] && release "$r"; done
[ $DRY = 1 ] && echo "dry run: nothing written"
exit 0
