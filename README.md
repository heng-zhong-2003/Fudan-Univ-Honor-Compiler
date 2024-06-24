要运行该编译器，按照如下方法：

1. `cd` 到项目根目录；
2. 构建：`make build`；
3. 把需要编译和运行的 FDMJ 源文件（扩展名 `.fmj`）放到 `test` 文件夹中，然后按照目的进行一下操作：
   - 仅编译：`make compile`。注意，该指令默认的 arch size 是 8，如果想要仅编译得到 ARM 汇编文件，可以改 Makefile 中主程序的命令行参数为 4；
   - 编译到并运行 LLVM-IR：`make run-llvm`；
   - 编译到并运行 ARM 汇编：`make run-arm`。
