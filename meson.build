project(
    'mtm',
    'c',
    'cpp',
    version: '1.2.1',
    default_options: ['c_std=c99'],
)

libvterm_dep = dependency('libvterm')
dependencies = [libvterm_dep]
mtm_args = []
mtm_srcs = [
    'main.cpp',
    'config.c',
    'node.cpp',
]
if host_machine.system() == 'windows'
    mtm_srcs += [
        'win32_term.cpp',
        'win32_process.cpp',
        'win32_screen.cpp',
        'win32_stream.cpp',
    ]
else
    ncurses_dep = dependency('ncursesw')
    dependencies += ncurses_dep
    mtm_srcs += [
        'vtparser.c',
        'mtm.cpp',
        'posix_selector.cpp',
        'posix_process.cpp',
        'curses_term.cpp',
        'curses_screen.cpp',
    ]
    mtm_args += [
        '-D_POSIX_C_SOURCE=200809L',
        '-D_XOPEN_SOURCE=600',
        '-D_XOPEN_SOURCE_EXTENDED',
    ]
endif

executable(
    'mtm',
    mtm_srcs,
    install: true,
    c_args: mtm_args,
    dependencies: dependencies,
)
