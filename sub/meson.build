plog_dep = dependency('plog')
curses_dep = dependency('curses')

sub_inc = include_directories('.')
sub_lib = static_library(
    'sub',
    [
        'app.cpp',
        'curses_term.cpp',
        'curses_window.cpp',
        'node.cpp',
        'pair.cpp',
        'selector.cpp',
        'vtcontext.cpp',
        'vthandler.cpp',
        'vtparser.cpp',
    ],
    include_directories: sub_inc,
    dependencies: [plog_dep, curses_dep],
)

sub_dep = declare_dependency(
    include_directories: sub_inc,
    link_with: sub_lib,
)
