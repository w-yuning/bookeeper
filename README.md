# Bookeeper 桌面记账应用

## 项目简介
Bookeeper 是基于 Qt5 Widgets/Charts 与 C++17 开发的本地单机记账应用，覆盖课程实验要求的全部模块：

- **用户与账户**：支持注册、登录、本地 JSON 数据持久化，多用户互不干扰。
- **记账与交易记录**：增删改查账单，类型/分类/时间/备注可编辑。
- **分类与统计**：自定义分类，仪表盘提供分类饼图与近 7 天支出柱状图。
- **推送与提醒**：用户可维护提醒列表，提醒到期时弹窗提示。
- **个性化与社交**：发布动态、互加好友，时间线展示公开或好友可见的动态与评论。

实现依据实验二设计文档与 UML 图 (`lab2/UML/*.puml`)，核心业务逻辑集中在 `src/core` 目录，约 520 行。

## 目录结构
```
code/
├── CMakeLists.txt         # CMake 构建脚本
├── README.md              # 当前说明文档
└── src/
    ├── core/              # 纯业务逻辑（实体、存储、服务）
    │   ├── Entities.*     # 领域实体与 JSON 序列化
    │   ├── JsonStorage.*  # 本地 JSON 文件存储
    │   └── LedgerService.*# 核心业务服务
    ├── ui/                # Qt Widgets 界面
    │   ├── BillEditorDialog.*
    │   ├── LoginWindow.*
    │   ├── MainWindow.*
    │   └── ReminderDialog.*
    └── main.cpp           # 应用入口
```

### 建议的阅读顺序
1. `core/Entities.*`：熟悉数据结构与序列化格式。
2. `core/JsonStorage.*`：了解本地持久化实现。
3. `core/LedgerService.*`：掌握业务规则（注册、账单、提醒、社交等）。
4. `ui/LoginWindow.*` 与 `ui/MainWindow.*`：查看界面交互与服务调用方式。
5. 结合 `lab2/实验报告.md` 与 UML 图掌握整体设计背景。

## 构建与运行
以下命令请在 *x64 Native Tools Command Prompt for VS 2022*（或等效 MSVC 环境）执行：

```powershell
cmake -S code -B code/build -G "Visual Studio 17 2022" -A x64
cmake --build code/build --config Debug
.code\build\Debug\bookeeper.exe
```

- Release 版本只需将 `Debug` 替换为 `Release`。
- 用户数据默认写入 `%AppData%/BookeeperLab/bookeeper_data/`，可删除对应 JSON 文件以重置账户。

## 编码规范检查
课程要求遵循 Google C++ Style Guide。推荐工具：

- **cpplint**：Google 官方风格检查脚本。安装 `pip install cpplint` 后，可在 `code/src` 下执行
  ```powershell
  cpplint --recursive core ui
  ```
  或使用 `python -m cpplint ...` 形式。
- （可选）`clang-format` 用于自动格式化，但请以 `cpplint` 为主进行风格核查。

运行 `cpplint` 前，请确保已安装 Python 3 与 pip。若后续实验需要提交风格检查结果，可将工具输出记录在实验报告中。

## 其他说明
- 若想观察提醒弹窗体验，可在应用中新增提醒并设置为当前时间附近；界面会在一分钟内弹窗提示。
- 社交模块的公开动态任何用户均可看到；好友动态需双方互加好友。
- 若需扩展或编写测试，可在 `src` 平级新增 `tests/` 目录并自定义 CMake 目标。
