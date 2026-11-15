# Changelog

## 0.1.0 (2026-11-15)

First public release.

- Shows the active keyboard layout as a coloured badge in any StatusNotifierItem tray, such as wf-panel-pi.
- Left click switches to the next layout, right click to the previous one, middle click back to the first.
- Labels and flag-based colours for all 98 standard xkb layouts, labels in the layout's own script where common
  fonts cover it (СР, РУ, ΕΛ, עב, ქა), and automatic handling of look-alikes (US/UK, EN/DV, SR1/SR2).
- Switching uses the dedicated "next layout" key, so it works whatever layout shortcut is configured.
- Debian packages for arm64, armhf and amd64, and a one-line installer.
