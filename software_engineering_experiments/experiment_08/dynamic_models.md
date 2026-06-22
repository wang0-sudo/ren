# 人才招聘系统动态模型

## 1. 个人用户审核状态图

```mermaid
stateDiagram-v2
    [*] --> NotRegistered
    NotRegistered --> Pending: registerPersonalUser()
    Pending --> Approved: reviewPersonalUser(Approved)
    Pending --> Rejected: reviewPersonalUser(Rejected)
    Approved --> Approved: updatePersonalInfo()/updatePersonalResume()
    Approved --> Approved: applyForPosition()/withdrawApplication()
    Rejected --> Rejected: login/view status
```

说明：

1. 个人用户注册后初始状态为 `Pending`。
2. 管理员审核后状态变为 `Approved` 或 `Rejected`。
3. `Approved` 用户可以维护资料、维护简历、申请岗位和撤销申请。
4. 申请岗位时仍需检查岗位是否可见以及是否重复申请。

## 2. 企业用户审核状态图

```mermaid
stateDiagram-v2
    [*] --> NotRegistered
    NotRegistered --> Pending: registerEnterpriseUser()
    Pending --> Approved: reviewEnterpriseUser(Approved)
    Pending --> Rejected: reviewEnterpriseUser(Rejected)
    Approved --> Approved: updateEnterpriseInfo()
    Approved --> Approved: addPosition()/updatePosition()/removePosition()
    Rejected --> Rejected: login/view status
```

说明：

1. 企业用户注册后初始状态为 `Pending`。
2. 只有 `Approved` 企业用户可以发布岗位。
3. 企业用户只能修改或下架自己发布且仍处于启用状态的岗位。

## 3. 岗位生命周期状态图

```mermaid
stateDiagram-v2
    [*] --> Created: addPosition()
    Created --> Active: active = true
    Active --> Visible: enterprise.status == Approved
    Visible --> Applied: applyForPosition()
    Applied --> Visible: withdrawApplication()
    Active --> Inactive: removePosition()
    Visible --> Inactive: removePosition()
    Inactive --> [*]
```

说明：

1. 岗位创建后 `active = true`。
2. 岗位对个人用户可见需要同时满足 `active = true` 和发布企业已审核通过。
3. 下架岗位设置 `active = false`，不再进入可见岗位列表。

## 4. 留言处理状态图

```mermaid
stateDiagram-v2
    [*] --> Draft
    Draft --> Pending: addMessage()
    Pending --> Handled: replyMessage()
    Pending --> Deleted: deleteMessage()
    Handled --> Deleted: deleteMessage()
    Deleted --> [*]
```

说明：

1. 个人用户和企业用户均可提交留言。
2. 新留言默认 `handled = false`。
3. 管理员回复后保存回复内容并设置 `handled = true`。
4. 管理员可以删除未处理或已处理留言。

## 5. 个人用户申请岗位活动图

```mermaid
flowchart TD
    Start([开始]) --> Login[个人用户登录]
    Login --> CheckUser{个人审核通过?}
    CheckUser -- 否 --> RejectUser[提示账号未审核通过]
    RejectUser --> End([结束])

    CheckUser -- 是 --> LoadPositions[读取可见岗位列表]
    LoadPositions --> SelectPosition[选择岗位]
    SelectPosition --> CheckPosition{岗位可见?}
    CheckPosition -- 否 --> RejectPosition[提示岗位不可申请]
    RejectPosition --> End

    CheckPosition -- 是 --> CheckDuplicate{是否已申请?}
    CheckDuplicate -- 是 --> RejectDuplicate[提示重复申请]
    RejectDuplicate --> End

    CheckDuplicate -- 否 --> AddApplication[写入 appliedPositionIds]
    AddApplication --> Persist[保存 recruitment_data.txt]
    Persist --> Success[显示申请成功]
    Success --> End
```

## 6. 企业用户发布岗位活动图

```mermaid
flowchart TD
    Start([开始]) --> Login[企业用户登录]
    Login --> CheckEnterprise{企业审核通过?}
    CheckEnterprise -- 否 --> RejectEnterprise[提示账号未审核通过]
    RejectEnterprise --> End([结束])

    CheckEnterprise -- 是 --> FillPosition[填写岗位标题、地点、薪资、要求和描述]
    FillPosition --> CheckTitle{岗位标题非空?}
    CheckTitle -- 否 --> RejectTitle[提示岗位名称不能为空]
    RejectTitle --> End

    CheckTitle -- 是 --> CreatePosition[创建 Position 并设置 active = true]
    CreatePosition --> Persist[保存 recruitment_data.txt]
    Persist --> Success[显示发布成功]
    Success --> End
```

## 7. 数据流图

### 7.1 上下文数据流图

```mermaid
flowchart LR
    Personal[个人用户]
    Enterprise[企业用户]
    Admin[管理员]
    System((人才招聘系统))
    Data[(recruitment_data.txt)]

    Personal -->|注册、登录、简历、岗位申请、留言| System
    System -->|岗位列表、申请结果、留言状态| Personal

    Enterprise -->|注册、登录、企业资料、岗位信息、留言| System
    System -->|人才列表、岗位状态、留言状态| Enterprise

    Admin -->|审核、回复、删除| System
    System -->|待审核用户、留言列表| Admin

    System <--> Data
```

### 7.2 一层数据流图

```mermaid
flowchart TB
    Personal[个人用户]
    Enterprise[企业用户]
    Admin[管理员]

    P1((注册登录处理))
    P2((审核管理))
    P3((岗位管理))
    P4((申请管理))
    P5((留言管理))

    D1[(个人用户数据)]
    D2[(企业用户数据)]
    D3[(岗位数据)]
    D4[(留言数据)]
    D5[(本地数据文件)]

    Personal --> P1
    Enterprise --> P1
    Admin --> P1
    P1 <--> D1
    P1 <--> D2
    P1 <--> D5

    Admin --> P2
    P2 <--> D1
    P2 <--> D2
    P2 <--> D5

    Enterprise --> P3
    P3 <--> D2
    P3 <--> D3
    P3 <--> D5

    Personal --> P4
    P4 <--> D1
    P4 <--> D2
    P4 <--> D3
    P4 <--> D5

    Personal --> P5
    Enterprise --> P5
    Admin --> P5
    P5 <--> D4
    P5 <--> D5
```

## 8. OCL/逻辑约束

本系统未直接引入 OCL 执行引擎，以下约束以 OCL 风格和逻辑表达式记录，可作为 SRS、设计和测试用例的共同依据。

### 8.1 用户名唯一性

```text
context RecruitmentService
inv UniqueUsername:
  personalUsers.username ∩ enterpriseUsers.username = ∅
  and adminUsername not in personalUsers.username
  and adminUsername not in enterpriseUsers.username
```

### 8.2 个人用户岗位申请准入

```text
context applyForPosition(personalId, positionId)
pre PersonalApproved:
  personalById(personalId).status = Approved
pre PositionVisible:
  positionById(positionId).active = true
  and enterpriseById(position.enterpriseId).status = Approved
pre NotDuplicate:
  positionId not in personalById(personalId).appliedPositionIds
post ApplicationRecorded:
  positionId in personalById(personalId).appliedPositionIds
```

### 8.3 企业用户发布岗位准入

```text
context addPosition(enterpriseId, title, location, salary, requirement, description)
pre EnterpriseApproved:
  enterpriseById(enterpriseId).status = Approved
pre TitleRequired:
  title <> ""
post PositionCreated:
  positions includes new Position
  and new Position.active = true
```

### 8.4 岗位可见性

```text
context visiblePositions()
post VisibleOnly:
  result->forAll(p |
    p.active = true and enterpriseById(p.enterpriseId).status = Approved
  )
```

### 8.5 岗位修改与下架权限

```text
context updatePosition(enterpriseId, positionId, ...)
pre OwnActivePosition:
  positionById(positionId).enterpriseId = enterpriseId
  and positionById(positionId).active = true

context removePosition(enterpriseId, positionId)
pre OwnActivePosition:
  positionById(positionId).enterpriseId = enterpriseId
  and positionById(positionId).active = true
post PositionInactive:
  positionById(positionId).active = false
```

### 8.6 留言处理约束

```text
context addMessage(senderType, senderName, content)
pre ContentRequired:
  content <> ""
post MessagePending:
  new Message.handled = false

context replyMessage(messageId, reply)
pre ReplyRequired:
  reply <> ""
post MessageHandled:
  messageById(messageId).handled = true
  and messageById(messageId).reply = reply
```
