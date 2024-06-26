要运行该编译器，按照如下方法：

1. `cd` 到项目根目录；
2. 构建：`make build`；
3. 把需要编译和运行的 FDMJ 源文件（扩展名 `.fmj`）放到 `test` 文件夹中，然后按照目的进行一下操作：
   - 仅编译：`make compile`。注意，该指令默认的 arch size 是 8，如果想要仅编译得到 ARM 汇编文件，可以改 Makefile 中主程序的命令行参数为 4；
   - 编译到并运行 LLVM-IR：`make run-llvm`；
   - 编译到并运行 ARM 汇编：`make run-arm`。

**我不在意任何方式的使用，只要遵守 GPL3，也不承担任何责任。放到 GitHub 的本意就是为了下一届参考，毕竟计算机英才班的同学似乎不得不修这门课，然而强制使用 C 语言写编译器这种数据结构密集的项目是严重落后而完全没有道理的。我衷心希望下一届的这门课可以有所改观，至少能自选语言（想学 Standard ML 的话欢迎有问题问我）。无论如何，希望我的实现有所帮助。Lambda Prevails!**
