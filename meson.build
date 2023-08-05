project(
    'mtm',
    'c',
    'cpp',
    version: '1.2.1',
    default_options: ['c_std=c99'],
)

ncurses_dep = dependency('ncursesw')

executable(
    'mtm',
    ['main.cpp', 'mtm.cpp', 'pair.c', 'vtparser.c'],
    install: true,
    c_args: [
        '-D_POSIX_C_SOURCE=200809L',
        '-D_XOPEN_SOURCE=600',
        '-D_XOPEN_SOURCE_EXTENDED',
    ],
    dependencies: [ncurses_dep],
)
