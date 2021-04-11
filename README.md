[original](README.rst)

# mtm 改造

* CMakeLists.txt
* c++ 化。`C99` の文法がコンパイルできなくなったので、コンパイラを `clang` に変えた
* shared_ptr を導入して NODE の管理を簡略化
* WIP: select, NODE管理, view(curses), vtparser が疎結合になるように改造

# 構造

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
