project(
    'mtm',
    'c',
    'cpp',
    version: '1.2.1',
    default_options: ['c_std=c99', 'cpp_std=c++20'],
)

ncurses_dep = dependency('ncursesw')

executable(
    'mtm',
    [
        'main.cpp',
        'mtm.cpp',
        'pair.c',
        'vtparser.c',
        'config.c',
        'node.cpp',
        'posix_selector.cpp',
        'posix_process.cpp',
        'curses_term.cpp',
        'curses_screen.cpp',
    ],
    install: true,
    c_args: [
        '-D_POSIX_C_SOURCE=200809L',
        '-D_XOPEN_SOURCE=600',
        '-D_XOPEN_SOURCE_EXTENDED',
    ],
    dependencies: [ncurses_dep],
)
