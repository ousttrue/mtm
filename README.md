[original](README.rst)

# mtm 改造

* CMakeLists.txt
* c++ 化。`C99` の文法がコンパイルできなくなったので、コンパイラを `clang` に変えた
* 

# 構造

```
root(HORIZONTAL)
 +----+----+
 |    |    |
 +----+----+
   |     |
   v     v
+----+ +----+
|    | |    |
+----+ +----+
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
+--------------+
|VTParserが解釈|
+--------------+
```
