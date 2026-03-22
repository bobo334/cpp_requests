# cpp_requests

这是一个以 C++ 实现为主的仓库，核心代码位于 [`requests_cpp/`](requests_cpp/)。

## 构建（本地）

```bash
cmake -S requests_cpp -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

## 目录说明

- [`requests_cpp/include/`](requests_cpp/include/)：公开头文件
- [`requests_cpp/src/`](requests_cpp/src/)：实现代码
- [`requests_cpp/examples/`](requests_cpp/examples/)：示例程序
- [`requests_cpp/tests/`](requests_cpp/tests/)：测试代码

## 说明

仓库中的 Python 版本代码已移除，后续以 C++ 版本为主。
