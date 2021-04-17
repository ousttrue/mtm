[original](README.rst)

# mtm 改造

* CMakeLists.txt
* c++ 化。`C99` の文法がコンパイルできなくなったので、コンパイラを `clang` に変えた
* shared_ptr を導入して NODE の管理を簡略化
* WIP: select, NODE管理, view(curses), vtparser が疎結合になるように改造

# 概要

```
root(HORIZONTAL)
 +----+----+
 |    |    |
 +----+----+
   |     |    +- user input
   v     v    v
+----+ +------+
|    | | focus|
+----+ +------+
view   view
 |      |
 pty   pty
 |      |
 v      v
+---------+
| select  |
+---------+
  |read
  v
+--------+
|VTParser| update visual
+--------+
```

# データ構造

## Node

縦横のSplitterの木構造

## CursesTerm

SplitterのLeafにTerminalを実装

## VtParser

CussesTerm 上の pty からの出力を解釈する。

# 参考

* https://docs.microsoft.com/ja-jp/windows/console/console-virtual-terminal-sequences#example
* https://vt100.net/emu/dec_ansi_parser
* https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
