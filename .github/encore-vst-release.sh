#!/usr/bin/env bash
# Downloads one file of the ENCORE. VST release that external/encore-vst is pinned
# to, and checks it against its .sha256. Fails if the pin isn't a published release.
#
#   .github/encore-vst-release.sh <what> <folder>
#
# <what> is the end of the file name: windows-setup.exe, macos.pkg,
# linux-x64.tar.gz or linux-arm64.tar.gz. Prints the file's path.
set -euo pipefail
what="${1:?usage: encore-vst-release.sh <windows-setup.exe|macos.pkg|linux-x64.tar.gz|linux-arm64.tar.gz> <folder>}"
dir="${2:?folder}"
repo=https://github.com/Encore-Music-Inc/encore-vst
pin=$(git ls-tree HEAD external/encore-vst | awk '{ print $3 }')
tag=$(git ls-remote --tags "$repo.git" \
      | awk -v pin="$pin" '$1 == pin { sub("refs/tags/", "", $2); sub(/\^\{\}$/, "", $2); print $2 }' | grep '^v' | head -n 1 || true)
if [[ -z "$tag" ]]; then
  echo "::error::external/encore-vst (${pin:0:12}) is not a published ENCORE. VST release" >&2
  exit 1
fi
name="ENCORE-VST-${tag#v}-$what"
mkdir -p "$dir"
curl -fsSL --retry 3 -o "$dir/$name" "$repo/releases/download/$tag/$name"
curl -fsSL --retry 3 -o "$dir/$name.sha256" "$repo/releases/download/$tag/$name.sha256"
if command -v sha256sum >/dev/null; then (cd "$dir" && sha256sum -c "$name.sha256" >&2)
else (cd "$dir" && shasum -a 256 -c "$name.sha256" >&2); fi
echo "::notice title=ENCORE. VST::$name from $tag" >&2
echo "$dir/$name"
