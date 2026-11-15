#!/bin/sh
# Installs or updates labwc-layout-indicator from the latest GitHub release:
#   curl -fsSL https://raw.githubusercontent.com/b-baros/labwc-layout-indicator/main/install.sh | sh
set -eu

repo="b-baros/labwc-layout-indicator"
name="labwc-layout-indicator"
running="^(/usr/bin/)?$name( |\$)"

die() {
	echo "$name: $*" >&2
	exit 1
}

command -v dpkg >/dev/null 2>&1 ||
	die "this installer needs a Debian-based system such as Raspberry Pi OS. See the README to build from source."
arch=$(dpkg --print-architecture)
case "$arch" in
arm64 | amd64) ;;
armhf) [ "$(uname -m)" != armv6l ] || die "there is no prebuilt package for ARMv6 (Pi 1, Zero). See the README to build from source." ;;
*) die "there is no prebuilt package for $arch. See the README to build from source." ;;
esac

url=$(curl -fsSL "https://api.github.com/repos/$repo/releases/latest" |
	grep -o "https://[^\"]*/${name}_[^\"/]*_${arch}\.deb" | head -n 1)
[ -n "$url" ] || die "the latest release has no package for $arch"

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
chmod 755 "$tmp"
curl -fsSL -o "$tmp/$name.deb" "$url"
chmod 644 "$tmp/$name.deb"
echo "Installing ${url##*/}"
sudo apt-get install -y "$tmp/$name.deb" </dev/null

if [ -z "${WAYLAND_DISPLAY:-}" ]; then
	echo "Installed. It starts the next time you log in to the desktop."
	exit 0
fi

# Restart a running copy so the new version takes over; lwrespawn, if it
# started that copy, brings it back by itself within a second.
pkill -f "$running" 2>/dev/null || true
sleep 2
if ! pgrep -f "$running" >/dev/null 2>&1; then
	setsid "$name" >/dev/null 2>&1 </dev/null &
fi
echo "Installed and running. It also starts automatically every time you log in."
