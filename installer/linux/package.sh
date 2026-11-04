#!/usr/bin/env bash
# ENCORE. Basement Studio: the Linux packages, from a Release build.
#
#   installer/linux/package.sh <path to the built "Encore Studio"> <output folder> [deb|appimage|all]
#
# Makes, for the machine it runs on (x64 or arm64):
#   encore-basement-studio_<version>_<amd64|arm64>.deb
#     the app in /opt/encore-basement-studio, "encore-basement-studio" on the PATH,
#     a menu entry, icons, and .encore / .basement songs opening in it
#   Encore-Basement-Studio-<version>-<x86_64|aarch64>.AppImage
#     one file that runs on most distributions (built with a pinned linuxdeploy)
#
# Needs: dpkg-dev (dpkg-shlibdeps), rsvg-convert (librsvg2-bin), curl, file.
set -euo pipefail

say()  { echo "[package] $*"; }
fail() { echo "::error::[package] $*"; exit 1; }

[ $# -ge 2 ] || fail "usage: package.sh <Encore Studio binary> <output folder> [deb|appimage|all]"
bin=$(readlink -f "$1")
out=$(mkdir -p "$2" && readlink -f "$2")
what=${3:-all}
here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../.." && pwd)

[ -x "$bin" ] || fail "no app at $bin (build Release first)"
version=$(sed -nE 's/^project\(EncoreStudio VERSION ([0-9.]+)\)/\1/p' "$repo/CMakeLists.txt")
[ -n "$version" ] || fail "no project(EncoreStudio VERSION x.y.z) in CMakeLists.txt"

debarch=$(dpkg --print-architecture)
case "$debarch" in
    amd64) imgarch=x86_64 ;;
    arm64) imgarch=aarch64 ;;
    *) fail "unsupported architecture $debarch (x64 and arm64 only)" ;;
esac
say "version $version, arch $debarch / $imgarch, app $(du -h "$bin" | cut -f1)"

name=encore-basement-studio
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

# icons for the menu and for songs, drawn once into $work/share: the SVG plus PNGs at the usual sizes
icons() {
    local share="$work/share" s
    [ -d "$share/icons" ] && return 0
    install -Dm644 "$repo/Assets/encore-icon.svg" "$share/icons/hicolor/scalable/apps/$name.svg"
    for s in 16 24 32 48 64 128 256 512; do
        mkdir -p "$share/icons/hicolor/${s}x${s}/apps"
        rsvg-convert -w $s -h $s "$repo/Assets/encore-icon.svg" -o "$share/icons/hicolor/${s}x${s}/apps/$name.png" \
            || fail "rsvg-convert could not draw the ${s}px icon"
    done
}

make_deb() {
    local root="$work/deb" deb="$out/${name}_${version}_${debarch}.deb"
    say "deb: staging"
    install -Dm755 "$bin" "$root/opt/$name/Encore Studio"
    strip --strip-unneeded "$root/opt/$name/Encore Studio" || say "strip failed, keeping symbols"
    mkdir -p "$root/usr/bin"
    cat > "$root/usr/bin/$name" <<EOF
#!/bin/sh
# ENCORE. Basement Studio
exec "/opt/$name/Encore Studio" "\$@"
EOF
    chmod 755 "$root/usr/bin/$name"
    install -Dm644 "$here/$name.desktop" "$root/usr/share/applications/$name.desktop"
    install -Dm644 "$here/$name.xml" "$root/usr/share/mime/packages/$name.xml"
    icons
    cp -r "$work/share/icons" "$root/usr/share/"

    # copyright: Debian's machine-readable format, with the full AGPLv3 text
    local doc="$root/usr/share/doc/$name"
    mkdir -p "$doc"
    {
        echo "Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/"
        echo "Upstream-Name: ENCORE. Basement Studio"
        echo "Upstream-Contact: https://github.com/jimmyrlondo/encore-studio/issues"
        echo "Source: https://github.com/jimmyrlondo/encore-studio"
        echo "Comment: The ENCORE. name and logo belong to Encore Studio; forks are"
        echo " welcome under the license but should use their own name."
        echo
        echo "Files: *"
        echo "Copyright: 2026 Encore Studio"
        echo "License: AGPL-3"
        echo
        echo "Files: external/JUCE/*"
        echo "Copyright: Raw Material Software Limited"
        echo "License: AGPL-3"
        echo
        echo "Files: external/tracktion_engine/*"
        echo "Copyright: Tracktion Corporation"
        echo "License: GPL-3"
        echo " Tracktion Engine is used under the GNU General Public License v3."
        echo
        echo "Files: external/fst/*"
        echo "Copyright: IOhannes m zmoelnig"
        echo "License: GPL-3+"
        echo
        echo "Files: external/lame/*"
        echo "Copyright: The LAME project"
        echo "License: LGPL-2+"
        echo " On Debian systems the LGPL is in /usr/share/common-licenses/LGPL-2."
        echo
        echo "License: GPL-3"
        echo " On Debian systems the GPL v3 is in /usr/share/common-licenses/GPL-3."
        echo
        echo "License: GPL-3+"
        echo " On Debian systems the GPL v3 is in /usr/share/common-licenses/GPL-3."
        echo
        echo "License: AGPL-3"
        sed -e 's/^[[:space:]]*$/./' -e 's/^/ /' "$repo/LICENSE"
    } > "$doc/copyright"
    chmod 644 "$doc/copyright"

    # Depends: what the app links (dpkg-shlibdeps), plus the X11 libraries JUCE opens at run time
    say "deb: finding library dependencies"
    mkdir -p "$work/shlib/debian"
    printf 'Source: %s\n\nPackage: %s\nArchitecture: any\n' "$name" "$name" > "$work/shlib/debian/control"
    local linked
    linked=$(cd "$work/shlib" && dpkg-shlibdeps -O "$root/opt/$name/Encore Studio" 2>"$work/shlibdeps.err") \
        || { cat "$work/shlibdeps.err"; fail "dpkg-shlibdeps failed"; }
    linked=${linked#shlibs:Depends=}
    [ -n "$linked" ] || fail "dpkg-shlibdeps found no libraries"
    # Debian 13 / Ubuntu 24.04 renamed some libraries for 64-bit time (libasound2t64); older
    # releases (Debian 12, Ubuntu 22.04) still call them by the old name, so accept either
    linked=$(echo "$linked" | sed -E 's/(^|, )([a-z0-9.+-]+)t64( \([^)]*\))?/\1\2t64\3 | \2\3/g')
    local x11="libx11-6, libxext6, libxcursor1, libxinerama1, libxrender1, libxrandr2, libxi6"
    say "deb: Depends (linked): $linked"

    mkdir -p "$root/DEBIAN"
    cat > "$root/DEBIAN/control" <<EOF
Package: $name
Version: $version
Architecture: $debarch
Maintainer: ENCORE. Music <jimmyrlondo@users.noreply.github.com>
Installed-Size: $(du -sk --exclude=DEBIAN "$root" | cut -f1)
Depends: $linked, $x11
Recommends: libjack-jackd2-0 | libjack0
Suggests: pipewire-jack
Section: sound
Priority: optional
Homepage: https://encoremusic.studio
Description: ENCORE. Basement Studio, a free DAW for bands and home studios
 Record, edit and mix on your own computer, with the ENCORE. plugins
 (Vox, Verb, Equalize, Master and Strum) built in. Hosts your VST3, VST2
 and LADSPA plugins. Plays and records through ALSA, JACK or PipeWire.
EOF
    find "$root" -type d -exec chmod 755 {} +
    dpkg-deb --build --root-owner-group -Zxz "$root" "$deb" >/dev/null || fail "dpkg-deb failed"
    say "deb: built $(basename "$deb") ($(du -h "$deb" | cut -f1))"
    dpkg-deb --info "$deb" | sed 's/^/[package]   /'
}

# linuxdeploy, pinned to one release and checked against its SHA-256 (from GitHub's release listing)
LINUXDEPLOY_RELEASE=1-alpha-20251107-1
declare -A LINUXDEPLOY_SHA256=(
    [x86_64]=c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d
    [aarch64]=620095110d693282b8ebeb244a95b5e911cf8f65f76c88b4b47d16ae6346fcff
)

make_appimage() {
    local tool="$work/linuxdeploy-$imgarch.AppImage"
    local img="Encore-Basement-Studio-$version-$imgarch.AppImage"
    say "appimage: fetching linuxdeploy $LINUXDEPLOY_RELEASE ($imgarch)"
    curl -fsSL --retry 3 -o "$tool" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/$LINUXDEPLOY_RELEASE/linuxdeploy-$imgarch.AppImage" \
        || fail "could not download linuxdeploy $LINUXDEPLOY_RELEASE"
    echo "${LINUXDEPLOY_SHA256[$imgarch]}  $tool" | sha256sum -c --quiet - \
        || fail "linuxdeploy checksum mismatch (got $(sha256sum "$tool" | cut -d' ' -f1))"
    chmod +x "$tool"

    local app="$work/AppDir"
    install -Dm755 "$bin" "$app/usr/bin/$name"
    strip --strip-unneeded "$app/usr/bin/$name" || true
    install -Dm644 "$here/$name.xml" "$app/usr/share/mime/packages/$name.xml"
    icons

    say "appimage: building"
    # no FUSE needed: linuxdeploy (and the appimagetool inside it) unpack themselves
    ( cd "$work" && APPIMAGE_EXTRACT_AND_RUN=1 ARCH=$imgarch LINUXDEPLOY_OUTPUT_VERSION=$version OUTPUT="$img" \
        "$tool" --appdir "$app" \
            --executable "$app/usr/bin/$name" \
            --desktop-file "$here/$name.desktop" \
            --icon-file "$work/share/icons/hicolor/256x256/apps/$name.png" \
            --icon-file "$work/share/icons/hicolor/scalable/apps/$name.svg" \
            --output appimage ) > "$work/linuxdeploy.log" 2>&1 \
        || { tail -n 60 "$work/linuxdeploy.log"; fail "linuxdeploy failed"; }
    grep -iE "warn|error" "$work/linuxdeploy.log" | sed 's/^/[package]   linuxdeploy: /' || true
    [ -f "$work/$img" ] || { tail -n 30 "$work/linuxdeploy.log"; fail "linuxdeploy made no $img"; }
    mv "$work/$img" "$out/$img"
    say "appimage: bundled libraries: $(ls "$app/usr/lib" 2>/dev/null | tr '\n' ' ')"
    say "appimage: built $img ($(du -h "$out/$img" | cut -f1))"
}

case "$what" in
    deb) make_deb ;;
    appimage) make_appimage ;;
    all) make_deb; make_appimage ;;
    *) fail "unknown package type $what (deb, appimage or all)" ;;
esac
