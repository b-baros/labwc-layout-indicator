# Roadmap

Version 0.1 shows the layout, switches it by click and has predefined badges for every standard layout. Ideas for
later versions, roughly in order:

## Settings
- A configuration file, later a small settings window.
- Your own label and colour for any layout.
- Choose the default layout for middle click, and what each click does.

## Looks
- **Dark mode:** badges that adapt to a light or dark panel.
- **Flags:** optionally show the flag, or a strip of it, on the badge.
- **Caps Lock:** show on the badge when Caps Lock is on. The keyboard events the indicator already receives
  include the lock state.

## Interaction
- **Mouse wheel** to switch layouts.
- **Layout menu** listing all layouts to pick one directly (dbusmenu). wf-panel-pi opens a tray menu with the right
  button unless its `menu_on_middle_click` option is on. With that option, middle click would open the menu and
  right click would keep going to the previous layout.

## Setup
- A helper for more than two layouts on Raspberry Pi OS, whose Control Centre allows only two. It would write
  labwc's environment, the login screen's and `/etc/default/keyboard`.
- An APT repository on GitHub Pages, so updates arrive with `apt upgrade`.
- Translations of the tooltip.

## Reach
- A native wf-panel-pi plugin: no separate process and no D-Bus.
- Testing with other trays (Waybar, sfwbar, KDE Plasma), on Raspberry Pi OS bookworm and with other compositors.
