# 依赖注入学习笔记

## 1. 基本概念

依赖注入是一种对象创建和依赖管理方式。它的核心思想是：一个对象需要依赖其他对象时，不在自身内部直接创建依赖，而是由外部将依赖传入。

对比：

```cpp
// 不推荐：类内部创建依赖
class LoginWindow {
    RecruitmentService service_;
};
```

```cpp
// 更好：外部注入依赖
class LoginWindow {
public:
    explicit LoginWindow(std::shared_ptr<RecruitmentService> service);
private:
    std::shared_ptr<RecruitmentService> service_;
};
```

## 2. 依赖注入的目的

1. 降低模块和具体实现的耦合。
2. 让依赖关系在构造阶段清晰暴露。
3. 便于共享同一个服务对象。
4. 便于在测试中替换真实依赖。
5. 便于后续迁移到接口和多实现。

## 3. 常见注入方式

| 方式 | 说明 | 适用场景 |
|---|---|---|
| 构造函数注入 | 创建对象时传入依赖 | 依赖是必需的，推荐优先使用。 |
| Setter 注入 | 创建后通过 setter 设置依赖 | 依赖可选或需要运行时替换。 |
| 接口注入 | 通过接口方法注入依赖 | 使用较少，适合框架约定。 |
| 容器注入 | 由依赖注入容器创建和组装对象 | 大型项目或服务端系统。 |

本项目使用的是构造函数注入。

## 4. 当前项目中的依赖注入

### 4.1 服务创建位置

`main.cpp` 负责创建共享业务服务对象：

```cpp
auto service = std::make_shared<RecruitmentService>();
service->loadOrSeed();
LoginWindow loginWindow(service);
```

这样 `LoginWindow` 不需要知道服务对象如何创建，也不负责初始化数据。

### 4.2 登录窗口注入

```cpp
LoginWindow::LoginWindow(std::shared_ptr<RecruitmentService> service,
                         QWidget* parent)
    : QWidget(parent), service_(std::move(service)) {
    buildUi();
}
```

优点：

1. 登录窗口只负责登录和注册交互。
2. 服务对象来自外部，便于共享。
3. 后续可把真实服务替换为测试服务。

### 4.3 工作台窗口注入

```cpp
DashboardWindow(std::shared_ptr<RecruitmentService> service,
                UserRole role,
                int currentUserId,
                QWidget* parent = nullptr);
```

工作台窗口除服务外，还注入角色和当前用户编号。这使同一个 `DashboardWindow` 可以根据角色构建不同界面。

## 5. 当前设计收益

| 收益 | 说明 |
|---|---|
| 共享状态 | 登录窗口和工作台窗口使用同一个 `RecruitmentService` 实例。 |
| 职责清楚 | 窗口不负责创建和初始化服务。 |
| 生命周期明确 | 服务由 `main.cpp` 创建，窗口通过 `shared_ptr` 持有。 |
| 更容易演进 | 后续可以引入接口或测试替身。 |

## 6. 当前不足

### 6.1 依赖具体类

窗口依赖的是具体 `RecruitmentService`：

```cpp
std::shared_ptr<RecruitmentService> service_;
```

这意味着如果要替换为模拟服务，需要修改类型。

### 6.2 没有仓储接口

`RecruitmentService` 内部直接读写文本文件。业务规则和存储细节绑定在一起，测试时仍可能依赖真实文件。

### 6.3 缺少组合根约定

当前 `main.cpp` 实际上承担组合根职责，但文档和命名中没有明确说明。后续如果对象更多，可以单独整理依赖创建逻辑。

## 7. 改进方案

### 7.1 引入服务接口

```cpp
class IRecruitmentService {
public:
    virtual ~IRecruitmentService() = default;
    virtual bool loginAdmin(const std::string& username,
                            const std::string& password) const = 0;
    virtual bool applyForPosition(int personalId,
                                  int positionId,
                                  std::string& error) = 0;
};
```

窗口改为依赖：

```cpp
std::shared_ptr<IRecruitmentService> service_;
```

### 7.2 引入仓储接口

```cpp
class IRecruitmentRepository {
public:
    virtual ~IRecruitmentRepository() = default;
    virtual bool load(RecruitmentData& data) = 0;
    virtual bool save(const RecruitmentData& data) = 0;
};
```

`RecruitmentService` 通过构造函数接收仓储：

```cpp
RecruitmentService(std::shared_ptr<IRecruitmentRepository> repository);
```

### 7.3 测试替身

测试时可以注入内存仓储：

```cpp
auto repository = std::make_shared<InMemoryRecruitmentRepository>();
auto service = std::make_shared<RecruitmentService>(repository);
```

这样测试不需要操作真实 `recruitment_data.txt`。

## 8. 本项目推荐路线

课程提交阶段不必立即改代码，推荐按以下顺序推进：

1. 保持当前构造函数注入方式。
2. 先增加服务层测试。
3. 抽取仓储接口，将文件读写从服务层拆出。
4. 再抽象服务接口，支持测试替身和未来远程服务。
5. 最后考虑是否引入轻量 DI 容器。

## 9. 小结

当前项目已经具备依赖注入的基本形态：由 `main.cpp` 创建服务实例，并通过构造函数传入窗口类。该设计对课程项目已经足够清晰。后续真正需要提升的是“依赖抽象”和“存储拆分”，而不是过早引入复杂的依赖注入框架。
