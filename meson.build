project('labwc-layout-indicator', 'c',
	version: '0.1.0',
	license: 'MIT',
	meson_version: '>=0.60.0',
	default_options: ['c_std=c11', 'warning_level=2', 'buildtype=release'])

cc = meson.get_compiler('c')
add_project_arguments('-D_GNU_SOURCE', '-Wno-unused-parameter', language: 'c')

wayland_client = dependency('wayland-client')
xkbcommon = dependency('xkbcommon', version: '>=1.0.0')
math = cc.find_library('m', required: false)

sdbus_provider = get_option('sd-bus-provider')
sdbus = dependency('', required: false)
foreach name : sdbus_provider == 'auto' ? ['libsystemd', 'libelogind', 'basu'] : [sdbus_provider]
	if not sdbus.found()
		sdbus = dependency(name, required: false)
		if sdbus.found()
			sdbus_provider = name
		endif
	endif
endforeach
if not sdbus.found()
	error('sd-bus not found: install the development files of libsystemd, libelogind or basu')
endif

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
foreach name : ['libsystemd', 'libelogind', 'basu']
	conf.set10('HAVE_' + name.to_upper(), name == sdbus_provider)
endforeach
configure_file(output: 'config.h', configuration: conf)

src = include_directories('src')
core = static_library('core',
	'src/badges.c', 'src/keymap.c', 'src/layouts.c',
	dependencies: xkbcommon)

executable('labwc-layout-indicator',
	'src/main.c', 'src/state.c', 'src/switcher.c', 'src/tray.c', 'src/wayland.c',
	link_with: core,
	dependencies: [wayland_client, xkbcommon, sdbus],
	install: true)

executable('badge-sheet', 'tools/badge-sheet.c',
	include_directories: src,
	link_with: core,
	build_by_default: false)

install_data('data/labwc-layout-indicator.desktop', install_dir: get_option('autostart-dir'))
install_data('data/70-labwc-layout-indicator-uinput.rules', install_dir: get_option('udev-rules-dir'))

xkeyboard_config = dependency('xkeyboard-config', required: false)
xkb_base = xkeyboard_config.found() ? xkeyboard_config.get_variable(pkgconfig: 'xkb_base') : '/usr/share/X11/xkb'

test('keymap', executable('test-keymap', 'tests/test_keymap.c',
	include_directories: src, link_with: core, dependencies: xkbcommon))
test('badges', executable('test-badges', 'tests/test_badges.c',
	include_directories: src, link_with: core, dependencies: [xkbcommon, math]))
test('layout-coverage', executable('test-layout-coverage', 'tests/test_layout_coverage.c',
	include_directories: src, link_with: core,
	c_args: '-DEVDEV_XML="@0@"'.format(xkb_base / 'rules' / 'evdev.xml')))
