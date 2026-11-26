# 实验复现

从仓库根目录运行以下命令。数据范围为十进制、无补零、第一因子有 `m+1` 位、第二因子有 `m` 位，`2 <= m <= 12`。两份 CSV 采用 UTF-8，首行为列名，数对按 `(m,A,B)` 排序。[计数](counts.csv) · [因子对](solutions.csv)

## 构建

需要 C++17 和 Python 3.8 或更高版本（本次用 3.12.0）。只有交互求解器需要 Boost 头文件。两个枚举器可选用 OpenMP 分发首位或个位组合，串行构建同样覆盖全部组合。以下命令将构建产物放在 `build` 目录。

Linux 或有 GCC 的环境：

```sh
mkdir -p build
g++ -std=c++17 -O3 -pthread main.cpp -o build/solver
g++ -std=c++17 -O3 -fopenmp research/exact_unpadded_counts.cpp -o build/exact_unpadded_counts
g++ -std=c++17 -O3 -fopenmp research/interval_counter.cpp -o build/interval_counter
g++ -std=c++17 -O3 research/block_fibres.cpp -o build/block_fibres
```

Windows x64 Native Tools Command Prompt（将 Boost 路径改成含 `boost` 子目录的位置）：

```bat
if not exist build mkdir build
cl /nologo /std:c++17 /O2 /EHsc /MT /I"C:\path\to\boost-headers" main.cpp /Fobuild\solver.obj /Fe:build\solver.exe
cl /nologo /std:c++17 /O2 /EHsc /openmp research\exact_unpadded_counts.cpp /Fobuild\counts.obj /Fe:build\exact_unpadded_counts.exe
cl /nologo /std:c++17 /O2 /EHsc /openmp research\interval_counter.cpp /Fobuild\interval.obj /Fe:build\interval_counter.exe
cl /nologo /std:c++17 /O2 /EHsc research\block_fibres.cpp /Fobuild\fibres.obj /Fe:build\block_fibres.exe
```

## 核对解集

在 Windows 下，将下列可执行文件路径末尾加上 `.exe`；运行分块证书的命令为 `build\block_fibres.exe`。使用常规 Python 模式执行，脚本用 `assert` 核验断言。

```sh
python tests/test_solver.py build/solver
python research/verify_enumeration.py --counter build/exact_unpadded_counts --interval-counter build/interval_counter --prefix-max 7
python research/high_low_join.py build/solver
python research/n_plus_one_certificate.py
python research/line_incidence_certificate.py
./build/block_fibres
```

| 命令或程序 | 核验内容 |
| --- | --- |
| `tests/test_solver.py` | 全因子穷举 `(n,m)=(2,1),(3,2),(4,2),(5,2),(4,3)`，分别对照 1、8 线程下的两份完整解集。另核对 `n=2m`、`m=18,19,41` 的解集，以及一位乘数的区间输出。 |
| 同一脚本中的 `near_boundary` | 独立解线性同余，逐轮把后缀精度加倍，再用普通乘法核验。对于 `(n,m)=(5,3),(37,19),(12,7)`，得到的固定两位 / 无补零解数依次为 21 / 12、22 / 1、47 / 7。三个样例与交互程序输出的完整数对均一致。 |
| `verify_enumeration.py` | 直接按字符串拼接和普通乘法验算 40 组解。Python 高位区间搜索覆盖 2 至 7 位；两个 C++ 程序分别枚举 2 至 12 位，各自的完整解集均与公开 CSV 相等。低位程序还逐行对照叶子数。省略两个可执行参数可只做快速检查。 |
| `high_low_join.py` | 用整数直接扫描检查点与平面匹配，覆盖 `m=2..6`。2、3、4 位分别覆盖所有非空切分；5、6 位分别切在低 3、4 位。 |
| `n_plus_one_certificate.py` 和 `block_fibres` | 完整枚举有限后缀、共同因子与斜率条件，核验直方图最大值及分支数阈值。最大一张直方图有一千万个 64 位计数，约 80 MB（十进制）；运算时还要保留上一层。 |
| `line_incidence_certificate.py` | 对自由单列做有限穷举，抽查若干方向上的低位提升，并以有理数验证两个最终底数的取整。一般交线界依赖[证明](line_incidence_bound.md)。 |

分块公式另用 Python 大整数做了直接对照。取五组 24 位局部后缀，共同因子分别为 1、2、4、5、16。每组尝试全部 $58^2$ 个两位扩展。对 4 至 7 位块，从一个通过条件的扩展出发，把任意一列换成所有合法数位对。对每个样本，归一化同余的通过或拒绝结果都与普通乘法及字符串拼接一致。这项抽查检验进位公式，表中各类的最大分支数由穷尽系数和余数的有限证书给出。

低位计数器的两因子完整乘积在大规模时可能超出 64 位，但它只计算不超过约 $20\cdot10^{11}$ 的进位分子和不超过 13 位的因子。高位计数器直接维护残差，每一层存活节点满足 $0\le\delta\le a+b$，中间量保守估计也低于 $2\cdot10^{15}$。两者限定在源码的 12 位范围。两份文件均支持 `--list` 输出数对，`verify_enumeration.py` 会检查重复项和总数。对未覆盖的位数不能从这组实验推出无解。

## 与旧实现的实测对照

对照程序取自主分支历史版本 `0df558d8cb6cd3a88d4c9a17111cce3ed40e0f98`。本次在 Windows 10 x64、AMD Ryzen 9 7950X（16 核、32 逻辑处理器）、MSVC 19.44 x64、Boost 1.82 下，将两版用相同的 `/O2 /EHsc /MT` 设置重新编译。每个输入各运行三次，轮换两版先后次序。下表是从启动进程到进程退出的墙钟时间中位数，包含输出捕获。输入、两种规则的完整解集以及汇总均由脚本检查。比值只对应这台机器与这些输入。

| $n$ | $m$ | 线程 | 固定两位 / 无补零解数 | 旧版（秒） | 当前版（秒） | 旧 / 新 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 8 | 7 | 1 | 227 / 2 | 4.4649 | 0.3354 | 13.3 |
| 8 | 7 | 8 | 227 / 2 | 1.6083 | 0.0929 | 17.3 |
| 10 | 7 | 1 | 107 / 1 | 3.3511 | 0.0502 | 66.7 |

在上述 Windows 开发者命令提示符中可这样复现。脚本输出每次样本与中位数。在 Linux 下可使用相同的 `git show` 和 Python 命令，并用构建主程序的 GCC 参数编译旧源码。

```bat
git show 0df558d8cb6cd3a88d4c9a17111cce3ed40e0f98:main.cpp > build\original.cpp
cl /nologo /std:c++17 /O2 /EHsc /MT build\original.cpp /Fobuild\original.obj /Fe:build\original.exe
python research\compare_versions.py build\original.exe build\solver.exe
```

以上是交互求解器的性能数据。理论上的几何分割树没有在这组实验中实现或计时。
