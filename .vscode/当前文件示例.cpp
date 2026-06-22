#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace std;

enum class AuditStatus { Pending, Approved, Rejected };

namespace {
const char* const kDataFilePath = "招聘数据.txt";
const char* const kDataFileMagic = "RECRUITMENT_SYSTEM_DATA_V1";

AuditStatus auditStatusFromInt(int value) {
    switch (value) {
        case 0:
            return AuditStatus::Pending;
        case 1:
            return AuditStatus::Approved;
        case 2:
            return AuditStatus::Rejected;
        default:
            return AuditStatus::Pending;
    }
}
}  // namespace

struct Resume {
    string education;
    string experience;
    string skills;
    string selfIntroduction;
};

struct PersonalUser {
    int id = 0;
    string username;
    string password;
    string name;
    string gender;
    string phone;
    string email;
    Resume resume;
    vector<int> appliedPositionIds;
    AuditStatus status = AuditStatus::Pending;
};

struct EnterpriseUser {
    int id = 0;
    string username;
    string password;
    string companyName;
    string contactPerson;
    string phone;
    string email;
    string address;
    string introduction;
    AuditStatus status = AuditStatus::Pending;
};

struct Position {
    int id = 0;
    int enterpriseId = 0;
    string title;
    string location;
    string salary;
    string requirement;
    string description;
    bool active = true;
};

struct Message {
    int id = 0;
    string senderType;
    string senderName;
    string content;
    bool handled = false;
    string reply;
};

class RecruitmentSystem {
public:
    RecruitmentSystem();
    void run();

private:
    vector<PersonalUser> personalUsers;
    vector<EnterpriseUser> enterpriseUsers;
    vector<Position> positions;
    vector<Message> messages;

    string adminUsername = "admin";
    string adminPassword = "admin123";

    int nextPersonalId = 1001;
    int nextEnterpriseId = 2001;
    int nextPositionId = 3001;
    int nextMessageId = 4001;

    void seedData();
    bool loadData();
    bool saveData() const;
    void persistData();
    static string statusToString(AuditStatus status);
    static bool containsKeyword(const string& text, const string& keyword);

    int readInt(const string& prompt);
    string readLine(const string& prompt);
    void pause();

    PersonalUser* findPersonalByUsername(const string& username);
    EnterpriseUser* findEnterpriseByUsername(const string& username);
    EnterpriseUser* findEnterpriseById(int id);
    Position* findPositionById(int id);
    bool isPositionVisible(const Position& position);

    void personalPortal();
    void enterprisePortal();
    void adminPortal();

    void registerPersonalUser();
    void registerEnterpriseUser();
    void personalLogin();
    void enterpriseLogin();

    void personalMenu(PersonalUser& user);
    void showPersonalInfo(PersonalUser& user);
    void editPersonalInfo(PersonalUser& user);
    void editResume(PersonalUser& user);
    void printPosition(Position& position);
    void queryPositions();
    void jobManagement(PersonalUser& user);
    void changePassword(string& password);
    void leaveMessage(const string& senderType, const string& senderName);

    void enterpriseMenu(EnterpriseUser& user);
    void showEnterpriseInfo(EnterpriseUser& user);
    void editEnterpriseInfo(EnterpriseUser& user);
    void listEnterprisePositions(EnterpriseUser& user);
    void addPosition(EnterpriseUser& user);
    void editPosition(EnterpriseUser& user);
    void removePosition(EnterpriseUser& user);
    void managePositions(EnterpriseUser& user);
    void printResumeSummary(PersonalUser& user);
    void searchTalents(EnterpriseUser& user);

    void adminMenu();
    void reviewPersonalUsers();
    void reviewEnterpriseUsers();
    void auditMembers();
    void showMessages();
    void replyMessage();
    void deleteMessage();
    void messageManagement();
};

RecruitmentSystem::RecruitmentSystem() {
    if (!loadData()) {
        seedData();
        saveData();
    }
}

bool RecruitmentSystem::loadData() {
    ifstream in(kDataFilePath);
    if (!in) {
        return false;
    }

    string magic;
    if (!getline(in, magic) || magic != kDataFileMagic) {
        return false;
    }

    int loadedNextPersonalId = 0;
    int loadedNextEnterpriseId = 0;
    int loadedNextPositionId = 0;
    int loadedNextMessageId = 0;
    string loadedAdminPassword;
    size_t personalCount = 0;
    size_t enterpriseCount = 0;
    size_t positionCount = 0;
    size_t messageCount = 0;

    if (!(in >> loadedNextPersonalId >> loadedNextEnterpriseId >> loadedNextPositionId >>
          loadedNextMessageId >> quoted(loadedAdminPassword))) {
        return false;
    }

    if (!(in >> personalCount >> enterpriseCount >> positionCount >> messageCount)) {
        return false;
    }

    vector<PersonalUser> loadedPersonalUsers;
    vector<EnterpriseUser> loadedEnterpriseUsers;
    vector<Position> loadedPositions;
    vector<Message> loadedMessages;

    for (size_t i = 0; i < personalCount; ++i) {
        PersonalUser user;
        int statusValue = 0;
        size_t appliedCount = 0;
        if (!(in >> user.id >> quoted(user.username) >> quoted(user.password) >> quoted(user.name) >>
              quoted(user.gender) >> quoted(user.phone) >> quoted(user.email) >>
              quoted(user.resume.education) >> quoted(user.resume.experience) >>
              quoted(user.resume.skills) >> quoted(user.resume.selfIntroduction) >>
              statusValue >> appliedCount)) {
            return false;
        }

        user.status = auditStatusFromInt(statusValue);
        for (size_t j = 0; j < appliedCount; ++j) {
            int positionId = 0;
            if (!(in >> positionId)) {
                return false;
            }
            user.appliedPositionIds.push_back(positionId);
        }
        loadedPersonalUsers.push_back(user);
    }

    for (size_t i = 0; i < enterpriseCount; ++i) {
        EnterpriseUser user;
        int statusValue = 0;
        if (!(in >> user.id >> quoted(user.username) >> quoted(user.password) >>
              quoted(user.companyName) >> quoted(user.contactPerson) >> quoted(user.phone) >>
              quoted(user.email) >> quoted(user.address) >> quoted(user.introduction) >>
              statusValue)) {
            return false;
        }

        user.status = auditStatusFromInt(statusValue);
        loadedEnterpriseUsers.push_back(user);
    }

    for (size_t i = 0; i < positionCount; ++i) {
        Position position;
        if (!(in >> position.id >> position.enterpriseId >> quoted(position.title) >>
              quoted(position.location) >> quoted(position.salary) >>
              quoted(position.requirement) >> quoted(position.description) >>
              position.active)) {
            return false;
        }
        loadedPositions.push_back(position);
    }

    for (size_t i = 0; i < messageCount; ++i) {
        Message message;
        if (!(in >> message.id >> quoted(message.senderType) >> quoted(message.senderName) >>
              quoted(message.content) >> message.handled >> quoted(message.reply))) {
            return false;
        }
        loadedMessages.push_back(message);
    }

    personalUsers = move(loadedPersonalUsers);
    enterpriseUsers = move(loadedEnterpriseUsers);
    positions = move(loadedPositions);
    messages = move(loadedMessages);
    nextPersonalId = loadedNextPersonalId;
    nextEnterpriseId = loadedNextEnterpriseId;
    nextPositionId = loadedNextPositionId;
    nextMessageId = loadedNextMessageId;
    adminPassword = loadedAdminPassword;
    return true;
}

bool RecruitmentSystem::saveData() const {
    ofstream out(kDataFilePath, ios::out | ios::trunc);
    if (!out) {
        return false;
    }

    out << kDataFileMagic << '\n';
    out << nextPersonalId << ' ' << nextEnterpriseId << ' ' << nextPositionId << ' '
        << nextMessageId << ' ' << quoted(adminPassword) << '\n';
    out << personalUsers.size() << ' ' << enterpriseUsers.size() << ' ' << positions.size() << ' '
        << messages.size() << '\n';

    for (const auto& user : personalUsers) {
        out << user.id << ' ' << quoted(user.username) << ' ' << quoted(user.password) << ' '
            << quoted(user.name) << ' ' << quoted(user.gender) << ' ' << quoted(user.phone) << ' '
            << quoted(user.email) << ' ' << quoted(user.resume.education) << ' '
            << quoted(user.resume.experience) << ' ' << quoted(user.resume.skills) << ' '
            << quoted(user.resume.selfIntroduction) << ' ' << static_cast<int>(user.status) << ' '
            << user.appliedPositionIds.size();
        for (int positionId : user.appliedPositionIds) {
            out << ' ' << positionId;
        }
        out << '\n';
    }

    for (const auto& user : enterpriseUsers) {
        out << user.id << ' ' << quoted(user.username) << ' ' << quoted(user.password) << ' '
            << quoted(user.companyName) << ' ' << quoted(user.contactPerson) << ' '
            << quoted(user.phone) << ' ' << quoted(user.email) << ' ' << quoted(user.address) << ' '
            << quoted(user.introduction) << ' ' << static_cast<int>(user.status) << '\n';
    }

    for (const auto& position : positions) {
        out << position.id << ' ' << position.enterpriseId << ' ' << quoted(position.title) << ' '
            << quoted(position.location) << ' ' << quoted(position.salary) << ' '
            << quoted(position.requirement) << ' ' << quoted(position.description) << ' '
            << position.active << '\n';
    }

    for (const auto& message : messages) {
        out << message.id << ' ' << quoted(message.senderType) << ' ' << quoted(message.senderName)
            << ' ' << quoted(message.content) << ' ' << message.handled << ' '
            << quoted(message.reply) << '\n';
    }

    return static_cast<bool>(out);
}

void RecruitmentSystem::persistData() {
    if (!saveData()) {
        cout << "警告：数据保存失败，本次修改可能不会保留。\n";
    }
}

string RecruitmentSystem::statusToString(AuditStatus status) {
    switch (status) {
        case AuditStatus::Pending:
            return "待审核";
        case AuditStatus::Approved:
            return "已通过";
        case AuditStatus::Rejected:
            return "已驳回";
    }
    return "未知";
}

bool RecruitmentSystem::containsKeyword(const string& text, const string& keyword) {
    return keyword.empty() || text.find(keyword) != string::npos;
}

int RecruitmentSystem::readInt(const string& prompt) {
    while (true) {
        cout << prompt;
        int value = 0;
        if (cin >> value) {
            cin.ignore((numeric_limits<streamsize>::max)(), '\n');
            return value;
        }

        cin.clear();
        cin.ignore((numeric_limits<streamsize>::max)(), '\n');
        cout << "请输入有效的整数。\n";
    }
}

string RecruitmentSystem::readLine(const string& prompt) {
    cout << prompt;
    string value;
    getline(cin, value);
    return value;
}

void RecruitmentSystem::pause() {
    cout << "\n按回车键继续...";
    string line;
    getline(cin, line);
}

void RecruitmentSystem::seedData() {
    PersonalUser personal;
    personal.id = nextPersonalId++;
    personal.username = "alice";
    personal.password = "123456";
    personal.name = "张琳";
    personal.gender = "女";
    personal.phone = "13800000001";
    personal.email = "alice@example.com";
    personal.resume.education = "计算机科学本科";
    personal.resume.experience = "2年 C++ 后端开发经验";
    personal.resume.skills = "C++、STL、MySQL、Linux";
    personal.resume.selfIntroduction = "希望从事后端系统开发相关岗位。";
    personal.status = AuditStatus::Approved;
    personalUsers.push_back(personal);

    PersonalUser pendingPersonal;
    pendingPersonal.id = nextPersonalId++;
    pendingPersonal.username = "bob";
    pendingPersonal.password = "123456";
    pendingPersonal.name = "李明";
    pendingPersonal.gender = "男";
    pendingPersonal.phone = "13800000002";
    pendingPersonal.email = "bob@example.com";
    pendingPersonal.resume.education = "软件工程本科";
    pendingPersonal.resume.experience = "有 Web 开发实习经历";
    pendingPersonal.resume.skills = "Java、Spring、SQL";
    pendingPersonal.resume.selfIntroduction = "正在寻找初级开发岗位。";
    pendingPersonal.status = AuditStatus::Pending;
    personalUsers.push_back(pendingPersonal);

    EnterpriseUser enterprise;
    enterprise.id = nextEnterpriseId++;
    enterprise.username = "futurehr";
    enterprise.password = "123456";
    enterprise.companyName = "未来科技";
    enterprise.contactPerson = "何琳";
    enterprise.phone = "020-88886666";
    enterprise.email = "hr@futuretech.com";
    enterprise.address = "深圳";
    enterprise.introduction = "一家专注于人工智能与云产品研发的科技公司。";
    enterprise.status = AuditStatus::Approved;
    enterpriseUsers.push_back(enterprise);

    EnterpriseUser pendingEnterprise;
    pendingEnterprise.id = nextEnterpriseId++;
    pendingEnterprise.username = "greenhr";
    pendingEnterprise.password = "123456";
    pendingEnterprise.companyName = "绿源智造";
    pendingEnterprise.contactPerson = "陈杰";
    pendingEnterprise.phone = "0755-77889900";
    pendingEnterprise.email = "contact@greenstartup.com";
    pendingEnterprise.address = "广州";
    pendingEnterprise.introduction = "一家处于成长阶段的智能制造创业公司。";
    pendingEnterprise.status = AuditStatus::Pending;
    enterpriseUsers.push_back(pendingEnterprise);

    Position p1;
    p1.id = nextPositionId++;
    p1.enterpriseId = enterprise.id;
    p1.title = "C++ 开发工程师";
    p1.location = "深圳";
    p1.salary = "15k-25k";
    p1.requirement = "具备扎实的 C++ 基础，熟悉 STL";
    p1.description = "负责核心模块的开发与维护。";
    positions.push_back(p1);

    Position p2;
    p2.id = nextPositionId++;
    p2.enterpriseId = enterprise.id;
    p2.title = "后端工程师";
    p2.location = "深圳";
    p2.salary = "18k-30k";
    p2.requirement = "熟悉 Linux、数据库与网络基础";
    p2.description = "负责服务接口开发与后端性能优化。";
    positions.push_back(p2);

    Message message;
    message.id = nextMessageId++;
    message.senderType = "个人用户";
    message.senderName = "张琳";
    message.content = "希望系统中可以增加更多远程岗位。";
    messages.push_back(message);
}

PersonalUser* RecruitmentSystem::findPersonalByUsername(const string& username) {
    for (auto& user : personalUsers) {
        if (user.username == username) {
            return &user;
        }
    }
    return nullptr;
}

EnterpriseUser* RecruitmentSystem::findEnterpriseByUsername(const string& username) {
    for (auto& user : enterpriseUsers) {
        if (user.username == username) {
            return &user;
        }
    }
    return nullptr;
}

EnterpriseUser* RecruitmentSystem::findEnterpriseById(int id) {
    for (auto& user : enterpriseUsers) {
        if (user.id == id) {
            return &user;
        }
    }
    return nullptr;
}

Position* RecruitmentSystem::findPositionById(int id) {
    for (auto& position : positions) {
        if (position.id == id) {
            return &position;
        }
    }
    return nullptr;
}

bool RecruitmentSystem::isPositionVisible(const Position& position) {
    EnterpriseUser* enterprise = findEnterpriseById(position.enterpriseId);
    return position.active && enterprise != nullptr && enterprise->status == AuditStatus::Approved;
}

void RecruitmentSystem::run() {
    while (true) {
        cout << "\n================ 人才招聘系统 ================\n";
        cout << "1. 个人用户模块\n";
        cout << "2. 企业用户模块\n";
        cout << "3. 管理员模块\n";
        cout << "0. 退出系统\n";

        int choice = readInt("请选择：");
        switch (choice) {
            case 1:
                personalPortal();
                break;
            case 2:
                enterprisePortal();
                break;
            case 3:
                adminPortal();
                break;
            case 0:
                cout << "感谢使用，再见。\n";
                return;
            default:
                cout << "无效的选项。\n";
                pause();
                break;
        }
    }
}

void RecruitmentSystem::personalPortal() {
    while (true) {
        cout << "\n---------------- 个人用户入口 ----------------\n";
        cout << "1. 注册\n";
        cout << "2. 登录\n";
        cout << "0. 返回\n";

        int choice = readInt("请选择：");
        if (choice == 1) {
            registerPersonalUser();
        } else if (choice == 2) {
            personalLogin();
        } else if (choice == 0) {
            return;
        } else {
            cout << "无效的选项。\n";
            pause();
        }
    }
}

void RecruitmentSystem::enterprisePortal() {
    while (true) {
        cout << "\n--------------- 企业用户入口 ---------------\n";
        cout << "1. 注册\n";
        cout << "2. 登录\n";
        cout << "0. 返回\n";

        int choice = readInt("请选择：");
        if (choice == 1) {
            registerEnterpriseUser();
        } else if (choice == 2) {
            enterpriseLogin();
        } else if (choice == 0) {
            return;
        } else {
            cout << "无效的选项。\n";
            pause();
        }
    }
}

void RecruitmentSystem::adminPortal() {
    cout << "\n---------------- 管理员入口 ----------------\n";
    string username = readLine("管理员账号：");
    string password = readLine("管理员密码：");

    if (username == adminUsername && password == adminPassword) {
        adminMenu();
    } else {
        cout << "管理员账号或密码错误。\n";
        pause();
    }
}

void RecruitmentSystem::registerPersonalUser() {
    PersonalUser user;
    user.id = nextPersonalId++;
    user.username = readLine("用户名：");
    if (user.username.empty()) {
        cout << "用户名不能为空。\n";
        pause();
        return;
    }
    if (findPersonalByUsername(user.username) != nullptr) {
        cout << "用户名已存在。\n";
        pause();
        return;
    }

    user.password = readLine("密码：");
    user.name = readLine("姓名：");
    user.gender = readLine("性别：");
    user.phone = readLine("电话：");
    user.email = readLine("邮箱：");
    user.resume.education = readLine("学历：");
    user.resume.experience = readLine("工作经验：");
    user.resume.skills = readLine("技能特长：");
    user.resume.selfIntroduction = readLine("自我介绍：");
    personalUsers.push_back(user);
    persistData();

    cout << "注册成功，当前状态：待管理员审核。\n";
    pause();
}

void RecruitmentSystem::registerEnterpriseUser() {
    EnterpriseUser user;
    user.id = nextEnterpriseId++;
    user.username = readLine("用户名：");
    if (user.username.empty()) {
        cout << "用户名不能为空。\n";
        pause();
        return;
    }
    if (findEnterpriseByUsername(user.username) != nullptr) {
        cout << "用户名已存在。\n";
        pause();
        return;
    }

    user.password = readLine("密码：");
    user.companyName = readLine("企业名称：");
    user.contactPerson = readLine("联系人：");
    user.phone = readLine("电话：");
    user.email = readLine("邮箱：");
    user.address = readLine("地址：");
    user.introduction = readLine("企业简介：");
    enterpriseUsers.push_back(user);
    persistData();

    cout << "注册成功，当前状态：待管理员审核。\n";
    pause();
}

void RecruitmentSystem::personalLogin() {
    string username = readLine("用户名：");
    string password = readLine("密码：");

    PersonalUser* user = findPersonalByUsername(username);
    if (user == nullptr || user->password != password) {
        cout << "用户名或密码错误。\n";
        pause();
        return;
    }

    personalMenu(*user);
}

void RecruitmentSystem::enterpriseLogin() {
    string username = readLine("用户名：");
    string password = readLine("密码：");

    EnterpriseUser* user = findEnterpriseByUsername(username);
    if (user == nullptr || user->password != password) {
        cout << "用户名或密码错误。\n";
        pause();
        return;
    }

    enterpriseMenu(*user);
}

void RecruitmentSystem::showPersonalInfo(PersonalUser& user) {
    cout << "\n个人资料\n";
    cout << "编号：" << user.id << '\n';
    cout << "用户名：" << user.username << '\n';
    cout << "姓名：" << user.name << '\n';
    cout << "性别：" << user.gender << '\n';
    cout << "电话：" << user.phone << '\n';
    cout << "邮箱：" << user.email << '\n';
    cout << "审核状态：" << statusToString(user.status) << '\n';
    cout << "学历：" << user.resume.education << '\n';
    cout << "工作经验：" << user.resume.experience << '\n';
    cout << "技能特长：" << user.resume.skills << '\n';
    cout << "自我介绍：" << user.resume.selfIntroduction << '\n';
}

void RecruitmentSystem::editPersonalInfo(PersonalUser& user) {
    cout << "\n修改个人信息（直接回车可保留原值）\n";

    string value = readLine("姓名 [" + user.name + "]：");
    if (!value.empty()) {
        user.name = value;
    }
    value = readLine("性别 [" + user.gender + "]：");
    if (!value.empty()) {
        user.gender = value;
    }
    value = readLine("电话 [" + user.phone + "]：");
    if (!value.empty()) {
        user.phone = value;
    }
    value = readLine("邮箱 [" + user.email + "]：");
    if (!value.empty()) {
        user.email = value;
    }

    persistData();
    cout << "个人信息已更新。\n";
    pause();
}

void RecruitmentSystem::editResume(PersonalUser& user) {
    cout << "\n修改简历（直接回车可保留原值）\n";

    string value = readLine("学历 [" + user.resume.education + "]：");
    if (!value.empty()) {
        user.resume.education = value;
    }
    value = readLine("工作经验 [" + user.resume.experience + "]：");
    if (!value.empty()) {
        user.resume.experience = value;
    }
    value = readLine("技能特长 [" + user.resume.skills + "]：");
    if (!value.empty()) {
        user.resume.skills = value;
    }
    value = readLine("自我介绍 [" + user.resume.selfIntroduction + "]：");
    if (!value.empty()) {
        user.resume.selfIntroduction = value;
    }

    persistData();
    cout << "简历已更新。\n";
    pause();
}

void RecruitmentSystem::printPosition(Position& position) {
    EnterpriseUser* enterprise = findEnterpriseById(position.enterpriseId);
    cout << "\n岗位编号：" << position.id << '\n';
    cout << "企业名称：" << (enterprise == nullptr ? "未知企业" : enterprise->companyName) << '\n';
    cout << "岗位名称：" << position.title << '\n';
    cout << "工作地点：" << position.location << '\n';
    cout << "薪资待遇：" << position.salary << '\n';
    cout << "任职要求：" << position.requirement << '\n';
    cout << "岗位描述：" << position.description << '\n';
}

void RecruitmentSystem::queryPositions() {
    string keyword = readLine("请输入关键字（岗位/企业/地点），直接回车查看全部：");
    bool found = false;

    for (auto& position : positions) {
        if (!isPositionVisible(position)) {
            continue;
        }

        EnterpriseUser* enterprise = findEnterpriseById(position.enterpriseId);
        string companyName = enterprise == nullptr ? "" : enterprise->companyName;
        bool matched = containsKeyword(position.title, keyword) ||
                       containsKeyword(position.location, keyword) ||
                       containsKeyword(companyName, keyword) ||
                       containsKeyword(position.requirement, keyword);

        if (matched) {
            printPosition(position);
            found = true;
        }
    }

    if (!found) {
        cout << "未找到匹配的岗位。\n";
    }
    pause();
}

void RecruitmentSystem::jobManagement(PersonalUser& user) {
    if (user.status != AuditStatus::Approved) {
        cout << "您的账号尚未审核通过，暂时不能进行求职管理。\n";
        pause();
        return;
    }

    while (true) {
        cout << "\n---------------- 求职管理 ----------------\n";
        cout << "1. 查看已申请岗位\n";
        cout << "2. 申请岗位\n";
        cout << "3. 撤销申请\n";
        cout << "0. 返回\n";

        int choice = readInt("请选择：");
        if (choice == 0) {
            return;
        }

        if (choice == 1) {
            if (user.appliedPositionIds.empty()) {
                cout << "您还没有申请任何岗位。\n";
            } else {
                for (int positionId : user.appliedPositionIds) {
                    Position* position = findPositionById(positionId);
                    if (position != nullptr && isPositionVisible(*position)) {
                        printPosition(*position);
                    } else {
                        cout << "\n岗位编号：" << positionId << "（岗位已下架）\n";
                    }
                }
            }
            pause();
        } else if (choice == 2) {
            cout << "\n当前可申请岗位：\n";
            bool found = false;
            for (auto& position : positions) {
                if (isPositionVisible(position)) {
                    printPosition(position);
                    found = true;
                }
            }

            if (!found) {
                cout << "暂无可申请岗位。\n";
                pause();
                continue;
            }

            int positionId = readInt("请输入要申请的岗位编号：");
            Position* position = findPositionById(positionId);
            if (position == nullptr || !isPositionVisible(*position)) {
                cout << "岗位编号无效。\n";
                pause();
                continue;
            }

            if (find(user.appliedPositionIds.begin(), user.appliedPositionIds.end(), positionId) !=
                user.appliedPositionIds.end()) {
                cout << "您已经申请过该岗位。\n";
                pause();
                continue;
            }

            user.appliedPositionIds.push_back(positionId);
            persistData();
            cout << "申请提交成功。\n";
            pause();
        } else if (choice == 3) {
            int positionId = readInt("请输入要撤销的岗位编号：");
            auto it = find(user.appliedPositionIds.begin(), user.appliedPositionIds.end(), positionId);
            if (it == user.appliedPositionIds.end()) {
                cout << "未找到该申请记录。\n";
            } else {
                user.appliedPositionIds.erase(it);
                persistData();
                cout << "已撤销申请。\n";
            }
            pause();
        } else {
            cout << "无效的选项。\n";
            pause();
        }
    }
}

void RecruitmentSystem::changePassword(string& password) {
    string oldPassword = readLine("当前密码：");
    if (oldPassword != password) {
        cout << "当前密码不正确。\n";
        pause();
        return;
    }

    string newPassword = readLine("新密码：");
    if (newPassword.empty()) {
        cout << "新密码不能为空。\n";
        pause();
        return;
    }

    string confirmPassword = readLine("确认新密码：");
    if (newPassword != confirmPassword) {
        cout << "两次输入的密码不一致。\n";
        pause();
        return;
    }

    password = newPassword;
    persistData();
    cout << "密码修改成功。\n";
    pause();
}

void RecruitmentSystem::leaveMessage(const string& senderType, const string& senderName) {
    Message message;
    message.id = nextMessageId++;
    message.senderType = senderType;
    message.senderName = senderName;
    message.content = readLine("请输入留言内容：");

    if (message.content.empty()) {
        cout << "留言不能为空。\n";
        pause();
        return;
    }

    messages.push_back(message);
    persistData();
    cout << "留言提交成功。\n";
    pause();
}

void RecruitmentSystem::personalMenu(PersonalUser& user) {
    while (true) {
        cout << "\n================ 个人用户模块 ================\n";
        cout << "用户：" << user.username << " | 审核状态：" << statusToString(user.status) << '\n';
        cout << "1. 查看个人信息\n";
        cout << "2. 修改个人信息\n";
        cout << "3. 修改简历\n";
        cout << "4. 职位查询\n";
        cout << "5. 求职管理\n";
        cout << "6. 修改密码\n";
        cout << "7. 我要留言\n";
        cout << "0. 退出登录\n";

        int choice = readInt("请选择：");
        if (choice == 1) {
            showPersonalInfo(user);
            pause();
        } else if (choice == 2) {
            editPersonalInfo(user);
        } else if (choice == 3) {
            editResume(user);
        } else if (choice == 4) {
            queryPositions();
        } else if (choice == 5) {
            jobManagement(user);
        } else if (choice == 6) {
            changePassword(user.password);
        } else if (choice == 7) {
            leaveMessage("个人用户", user.name.empty() ? user.username : user.name);
        } else if (choice == 0) {
            return;
        } else {
            cout << "无效的选项。\n";
            pause();
        }
    }
}

void RecruitmentSystem::showEnterpriseInfo(EnterpriseUser& user) {
    cout << "\n企业资料\n";
    cout << "编号：" << user.id << '\n';
    cout << "用户名：" << user.username << '\n';
    cout << "企业名称：" << user.companyName << '\n';
    cout << "联系人：" << user.contactPerson << '\n';
    cout << "电话：" << user.phone << '\n';
    cout << "邮箱：" << user.email << '\n';
    cout << "地址：" << user.address << '\n';
    cout << "企业简介：" << user.introduction << '\n';
    cout << "审核状态：" << statusToString(user.status) << '\n';
}

void RecruitmentSystem::editEnterpriseInfo(EnterpriseUser& user) {
    cout << "\n修改企业信息（直接回车可保留原值）\n";

    string value = readLine("企业名称 [" + user.companyName + "]：");
    if (!value.empty()) {
        user.companyName = value;
    }
    value = readLine("联系人 [" + user.contactPerson + "]：");
    if (!value.empty()) {
        user.contactPerson = value;
    }
    value = readLine("电话 [" + user.phone + "]：");
    if (!value.empty()) {
        user.phone = value;
    }
    value = readLine("邮箱 [" + user.email + "]：");
    if (!value.empty()) {
        user.email = value;
    }
    value = readLine("地址 [" + user.address + "]：");
    if (!value.empty()) {
        user.address = value;
    }
    value = readLine("企业简介 [" + user.introduction + "]：");
    if (!value.empty()) {
        user.introduction = value;
    }

    persistData();
    cout << "企业信息已更新。\n";
    pause();
}

void RecruitmentSystem::listEnterprisePositions(EnterpriseUser& user) {
    bool found = false;
    for (auto& position : positions) {
        if (position.enterpriseId == user.id && position.active) {
            printPosition(position);
            found = true;
        }
    }

    if (!found) {
        cout << "当前没有发布岗位。\n";
    }
}

void RecruitmentSystem::addPosition(EnterpriseUser& user) {
    Position position;
    position.id = nextPositionId++;
    position.enterpriseId = user.id;
    position.title = readLine("岗位名称：");
    position.location = readLine("工作地点：");
    position.salary = readLine("薪资待遇：");
    position.requirement = readLine("任职要求：");
    position.description = readLine("岗位描述：");

    if (position.title.empty()) {
        cout << "岗位名称不能为空。\n";
        pause();
        return;
    }

    positions.push_back(position);
    persistData();
    cout << "岗位发布成功。\n";
    pause();
}

void RecruitmentSystem::editPosition(EnterpriseUser& user) {
    int positionId = readInt("请输入要修改的岗位编号：");
    Position* position = findPositionById(positionId);
    if (position == nullptr || position->enterpriseId != user.id || !position->active) {
        cout << "未找到该岗位。\n";
        pause();
        return;
    }

    string value = readLine("岗位名称 [" + position->title + "]：");
    if (!value.empty()) {
        position->title = value;
    }
    value = readLine("工作地点 [" + position->location + "]：");
    if (!value.empty()) {
        position->location = value;
    }
    value = readLine("薪资待遇 [" + position->salary + "]：");
    if (!value.empty()) {
        position->salary = value;
    }
    value = readLine("任职要求 [" + position->requirement + "]：");
    if (!value.empty()) {
        position->requirement = value;
    }
    value = readLine("岗位描述 [" + position->description + "]：");
    if (!value.empty()) {
        position->description = value;
    }

    persistData();
    cout << "岗位信息已更新。\n";
    pause();
}

void RecruitmentSystem::removePosition(EnterpriseUser& user) {
    int positionId = readInt("请输入要删除的岗位编号：");
    Position* position = findPositionById(positionId);
    if (position == nullptr || position->enterpriseId != user.id || !position->active) {
        cout << "未找到该岗位。\n";
        pause();
        return;
    }

    position->active = false;
    persistData();
    cout << "岗位已删除。\n";
    pause();
}

void RecruitmentSystem::managePositions(EnterpriseUser& user) {
    if (user.status != AuditStatus::Approved) {
        cout << "您的企业账号尚未审核通过，暂时不能进行岗位管理。\n";
        pause();
        return;
    }

    while (true) {
        cout << "\n-------------- 岗位信息管理 --------------\n";
        cout << "1. 查看当前岗位\n";
        cout << "2. 发布新岗位\n";
        cout << "3. 修改岗位\n";
        cout << "4. 删除岗位\n";
        cout << "0. 返回\n";

        int choice = readInt("请选择：");
        if (choice == 1) {
            listEnterprisePositions(user);
            pause();
        } else if (choice == 2) {
            addPosition(user);
        } else if (choice == 3) {
            listEnterprisePositions(user);
            editPosition(user);
        } else if (choice == 4) {
            listEnterprisePositions(user);
            removePosition(user);
        } else if (choice == 0) {
            return;
        } else {
            cout << "无效的选项。\n";
            pause();
        }
    }
}

void RecruitmentSystem::printResumeSummary(PersonalUser& user) {
    cout << "\n人才编号：" << user.id << '\n';
    cout << "姓名：" << user.name << '\n';
    cout << "性别：" << user.gender << '\n';
    cout << "电话：" << user.phone << '\n';
    cout << "邮箱：" << user.email << '\n';
    cout << "学历：" << user.resume.education << '\n';
    cout << "工作经验：" << user.resume.experience << '\n';
    cout << "技能特长：" << user.resume.skills << '\n';
    cout << "自我介绍：" << user.resume.selfIntroduction << '\n';
}

void RecruitmentSystem::searchTalents(EnterpriseUser& user) {
    if (user.status != AuditStatus::Approved) {
        cout << "您的企业账号尚未审核通过，暂时不能进行人才查询。\n";
        pause();
        return;
    }

    string keyword = readLine("请输入关键字（姓名/技能/经验），直接回车查看全部：");
    bool found = false;

    for (auto& talent : personalUsers) {
        if (talent.status != AuditStatus::Approved) {
            continue;
        }

        bool matched = containsKeyword(talent.name, keyword) ||
                       containsKeyword(talent.resume.skills, keyword) ||
                       containsKeyword(talent.resume.experience, keyword) ||
                       containsKeyword(talent.resume.education, keyword);

        if (matched) {
            printResumeSummary(talent);
            found = true;
        }
    }

    if (!found) {
        cout << "未找到匹配的人才信息。\n";
    }
    pause();
}

void RecruitmentSystem::enterpriseMenu(EnterpriseUser& user) {
    while (true) {
        cout << "\n=============== 企业用户模块 ===============\n";
        cout << "用户：" << user.username << " | 审核状态：" << statusToString(user.status) << '\n';
        cout << "1. 查看企业信息\n";
        cout << "2. 修改企业信息\n";
        cout << "3. 岗位信息管理\n";
        cout << "4. 人才查询\n";
        cout << "5. 修改密码\n";
        cout << "6. 我要留言\n";
        cout << "0. 退出登录\n";

        int choice = readInt("请选择：");
        if (choice == 1) {
            showEnterpriseInfo(user);
            pause();
        } else if (choice == 2) {
            editEnterpriseInfo(user);
        } else if (choice == 3) {
            managePositions(user);
        } else if (choice == 4) {
            searchTalents(user);
        } else if (choice == 5) {
            changePassword(user.password);
        } else if (choice == 6) {
            leaveMessage("企业用户", user.companyName.empty() ? user.username : user.companyName);
        } else if (choice == 0) {
            return;
        } else {
            cout << "无效的选项。\n";
            pause();
        }
    }
}

void RecruitmentSystem::reviewPersonalUsers() {
    cout << "\n个人会员审核列表\n";
    for (auto& user : personalUsers) {
        cout << "编号：" << user.id
             << " | 用户名：" << user.username
             << " | 姓名：" << user.name
             << " | 状态：" << statusToString(user.status) << '\n';
    }

    int userId = readInt("请输入要审核的用户编号（0 返回）：");
    if (userId == 0) {
        return;
    }

    for (auto& user : personalUsers) {
        if (user.id == userId) {
            cout << "1. 通过\n";
            cout << "2. 驳回\n";
            cout << "0. 取消\n";
            int action = readInt("请选择：");
            if (action == 1) {
                user.status = AuditStatus::Approved;
                persistData();
                cout << "个人用户已审核通过。\n";
            } else if (action == 2) {
                user.status = AuditStatus::Rejected;
                persistData();
                cout << "个人用户已驳回。\n";
            } else {
                cout << "已取消。\n";
            }
            pause();
            return;
        }
    }

    cout << "未找到该用户编号。\n";
    pause();
}

void RecruitmentSystem::reviewEnterpriseUsers() {
    cout << "\n企业会员审核列表\n";
    for (auto& user : enterpriseUsers) {
        cout << "编号：" << user.id
             << " | 用户名：" << user.username
             << " | 企业名称：" << user.companyName
             << " | 状态：" << statusToString(user.status) << '\n';
    }

    int userId = readInt("请输入要审核的企业编号（0 返回）：");
    if (userId == 0) {
        return;
    }

    for (auto& user : enterpriseUsers) {
        if (user.id == userId) {
            cout << "1. 通过\n";
            cout << "2. 驳回\n";
            cout << "0. 取消\n";
            int action = readInt("请选择：");
            if (action == 1) {
                user.status = AuditStatus::Approved;
                persistData();
                cout << "企业用户已审核通过。\n";
            } else if (action == 2) {
                user.status = AuditStatus::Rejected;
                persistData();
                cout << "企业用户已驳回。\n";
            } else {
                cout << "已取消。\n";
            }
            pause();
            return;
        }
    }

    cout << "未找到该企业编号。\n";
    pause();
}

void RecruitmentSystem::auditMembers() {
    while (true) {
        cout << "\n---------------- 会员审核管理 ----------------\n";
        cout << "1. 审核个人用户\n";
        cout << "2. 审核企业用户\n";
        cout << "0. 返回\n";

        int choice = readInt("请选择：");
        if (choice == 1) {
            reviewPersonalUsers();
        } else if (choice == 2) {
            reviewEnterpriseUsers();
        } else if (choice == 0) {
            return;
        } else {
            cout << "无效的选项。\n";
            pause();
        }
    }
}

void RecruitmentSystem::showMessages() {
    if (messages.empty()) {
        cout << "暂无留言。\n";
        return;
    }

    for (auto& message : messages) {
        cout << "\n留言编号：" << message.id << '\n';
        cout << "发送方类型：" << message.senderType << '\n';
        cout << "发送方名称：" << message.senderName << '\n';
        cout << "留言内容：" << message.content << '\n';
        cout << "处理状态：" << (message.handled ? "已处理" : "未处理") << '\n';
        if (message.handled) {
            cout << "回复内容：" << message.reply << '\n';
        }
    }
}

void RecruitmentSystem::replyMessage() {
    if (messages.empty()) {
        cout << "暂无留言。\n";
        pause();
        return;
    }

    showMessages();
    int messageId = readInt("请输入要回复的留言编号：");
    for (auto& message : messages) {
        if (message.id == messageId) {
            message.reply = readLine("回复内容：");
            if (message.reply.empty()) {
                cout << "回复内容不能为空。\n";
            } else {
                message.handled = true;
                persistData();
                cout << "留言已处理。\n";
            }
            pause();
            return;
        }
    }

    cout << "未找到该留言编号。\n";
    pause();
}

void RecruitmentSystem::deleteMessage() {
    if (messages.empty()) {
        cout << "暂无留言。\n";
        pause();
        return;
    }

    int messageId = readInt("请输入要删除的留言编号：");
    auto it = remove_if(messages.begin(), messages.end(),
                        [messageId](const Message& message) { return message.id == messageId; });

    if (it == messages.end()) {
        cout << "未找到该留言编号。\n";
    } else {
        messages.erase(it, messages.end());
        persistData();
        cout << "留言已删除。\n";
    }
    pause();
}

void RecruitmentSystem::messageManagement() {
    while (true) {
        cout << "\n---------------- 留言管理 ----------------\n";
        cout << "1. 查看全部留言\n";
        cout << "2. 回复留言\n";
        cout << "3. 删除留言\n";
        cout << "0. 返回\n";

        int choice = readInt("请选择：");
        if (choice == 1) {
            showMessages();
            pause();
        } else if (choice == 2) {
            replyMessage();
        } else if (choice == 3) {
            showMessages();
            deleteMessage();
        } else if (choice == 0) {
            return;
        } else {
            cout << "无效的选项。\n";
            pause();
        }
    }
}

void RecruitmentSystem::adminMenu() {
    while (true) {
        cout << "\n================ 管理员模块 ================\n";
        cout << "1. 会员审核管理\n";
        cout << "2. 留言管理\n";
        cout << "3. 修改密码\n";
        cout << "0. 退出登录\n";

        int choice = readInt("请选择：");
        if (choice == 1) {
            auditMembers();
        } else if (choice == 2) {
            messageManagement();
        } else if (choice == 3) {
            changePassword(adminPassword);
        } else if (choice == 0) {
            return;
        } else {
            cout << "无效的选项。\n";
            pause();
        }
    }
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    ios::sync_with_stdio(false);
    cin.tie(&cout);

    RecruitmentSystem system;
    system.run();
    return 0;
}
