# Qt 图形化版本

这个目录提供了一个基于 `Qt Widgets` 的招聘系统图形界面，和当前控制台版本并行存在。

## 当前状态

- 已包含登录窗口
- 已支持个人用户注册、企业用户注册
- 已包含个人用户、企业用户、管理员三类角色的主界面
- 已抽出一层独立的数据服务 `RecruitmentService`
- 数据服务可读取和生成 `recruitment_data.txt`
- 已在当前机器上用 `moc + g++` 实际编译出图形版程序

## 当前机器的 Qt 路径

- Qt: `D:\QT\6.10.2\mingw_64`
- MinGW: `D:\QT\Tools\mingw1310_64`
- Qt Creator: `D:\QT\Tools\QtCreator\bin\qtcreator.exe`

## 最简单的使用方式

### 1. 构建

```powershell
cd qt_gui
.\build_qt_gui.bat
```

### 2. 运行

```powershell
cd qt_gui
.\run_qt_gui.bat
```

## 生成结果

- 可执行文件：`qt_gui\build_manual\recruitment_qt_gui.exe`
- 构建脚本会同时复制 Qt/MinGW 运行库和 `platforms\qwindows.dll`
- 程序默认优先读取可执行文件同级或上一级目录中的 `recruitment_data.txt`

## 如果你想用 Qt Creator

可以直接用 Qt Creator 打开：

- `qt_gui\recruitment_qt_gui.pro`
或
- `qt_gui\CMakeLists.txt`

## 后续建议

- 先运行当前程序，确认登录、注册、表格和数据保存流程正常
- 再按需要补更细的功能，例如注册信息校验、详情弹窗、权限提示等
- 等 GUI 版本稳定后，再考虑把控制台版和图形版共用同一套业务层
