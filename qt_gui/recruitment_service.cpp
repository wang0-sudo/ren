#include "recruitment_service.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <utility>

namespace {
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

RecruitmentService::RecruitmentService(std::string dataFilePath)
    : dataFilePath_(std::move(dataFilePath)) {}

void RecruitmentService::loadOrSeed() {
    if (!load()) {
        seedData();
        save();
    }
}

bool RecruitmentService::registerPersonalUser(const std::string& username,
                                              const std::string& password,
                                              const std::string& name,
                                              const std::string& gender,
                                              const std::string& phone,
                                              const std::string& email,
                                              const Resume& resume,
                                              std::string& error) {
    if (username.empty()) {
        error = "用户名不能为空。";
        return false;
    }
    if (password.empty()) {
        error = "密码不能为空。";
        return false;
    }
    if (name.empty()) {
        error = "姓名不能为空。";
        return false;
    }
    if (usernameExists(username)) {
        error = "用户名已存在，请更换后重试。";
        return false;
    }

    PersonalUser user;
    user.id = nextPersonalId_++;
    user.username = username;
    user.password = password;
    user.name = name;
    user.gender = gender;
    user.phone = phone;
    user.email = email;
    user.resume = resume;
    user.status = AuditStatus::Pending;
    personalUsers_.push_back(user);
    return persist(error);
}

bool RecruitmentService::registerEnterpriseUser(const std::string& username,
                                                const std::string& password,
                                                const std::string& companyName,
                                                const std::string& contactPerson,
                                                const std::string& phone,
                                                const std::string& email,
                                                const std::string& address,
                                                const std::string& introduction,
                                                std::string& error) {
    if (username.empty()) {
        error = "用户名不能为空。";
        return false;
    }
    if (password.empty()) {
        error = "密码不能为空。";
        return false;
    }
    if (companyName.empty()) {
        error = "企业名称不能为空。";
        return false;
    }
    if (usernameExists(username)) {
        error = "用户名已存在，请更换后重试。";
        return false;
    }

    EnterpriseUser user;
    user.id = nextEnterpriseId_++;
    user.username = username;
    user.password = password;
    user.companyName = companyName;
    user.contactPerson = contactPerson;
    user.phone = phone;
    user.email = email;
    user.address = address;
    user.introduction = introduction;
    user.status = AuditStatus::Pending;
    enterpriseUsers_.push_back(user);
    return persist(error);
}

bool RecruitmentService::load() {
    std::ifstream in(dataFilePath_);
    if (!in) {
        return false;
    }

    std::string magic;
    if (!std::getline(in, magic) || magic != kDataFileMagic) {
        return false;
    }

    int loadedNextPersonalId = 0;
    int loadedNextEnterpriseId = 0;
    int loadedNextPositionId = 0;
    int loadedNextMessageId = 0;
    std::string loadedAdminPassword;
    std::size_t personalCount = 0;
    std::size_t enterpriseCount = 0;
    std::size_t positionCount = 0;
    std::size_t messageCount = 0;

    if (!(in >> loadedNextPersonalId >> loadedNextEnterpriseId >> loadedNextPositionId >>
          loadedNextMessageId >> std::quoted(loadedAdminPassword))) {
        return false;
    }

    if (!(in >> personalCount >> enterpriseCount >> positionCount >> messageCount)) {
        return false;
    }

    std::vector<PersonalUser> loadedPersonalUsers;
    std::vector<EnterpriseUser> loadedEnterpriseUsers;
    std::vector<Position> loadedPositions;
    std::vector<Message> loadedMessages;

    for (std::size_t i = 0; i < personalCount; ++i) {
        PersonalUser user;
        int statusValue = 0;
        std::size_t appliedCount = 0;

        if (!(in >> user.id >> std::quoted(user.username) >> std::quoted(user.password) >>
              std::quoted(user.name) >> std::quoted(user.gender) >> std::quoted(user.phone) >>
              std::quoted(user.email) >> std::quoted(user.resume.education) >>
              std::quoted(user.resume.experience) >> std::quoted(user.resume.skills) >>
              std::quoted(user.resume.selfIntroduction) >> statusValue >> appliedCount)) {
            return false;
        }

        user.status = auditStatusFromInt(statusValue);
        for (std::size_t j = 0; j < appliedCount; ++j) {
            int positionId = 0;
            if (!(in >> positionId)) {
                return false;
            }
            user.appliedPositionIds.push_back(positionId);
        }
        loadedPersonalUsers.push_back(user);
    }

    for (std::size_t i = 0; i < enterpriseCount; ++i) {
        EnterpriseUser user;
        int statusValue = 0;

        if (!(in >> user.id >> std::quoted(user.username) >> std::quoted(user.password) >>
              std::quoted(user.companyName) >> std::quoted(user.contactPerson) >>
              std::quoted(user.phone) >> std::quoted(user.email) >> std::quoted(user.address) >>
              std::quoted(user.introduction) >> statusValue)) {
            return false;
        }

        user.status = auditStatusFromInt(statusValue);
        loadedEnterpriseUsers.push_back(user);
    }

    for (std::size_t i = 0; i < positionCount; ++i) {
        Position position;
        if (!(in >> position.id >> position.enterpriseId >> std::quoted(position.title) >>
              std::quoted(position.location) >> std::quoted(position.salary) >>
              std::quoted(position.requirement) >> std::quoted(position.description) >>
              position.active)) {
            return false;
        }
        loadedPositions.push_back(position);
    }

    for (std::size_t i = 0; i < messageCount; ++i) {
        Message message;
        if (!(in >> message.id >> std::quoted(message.senderType) >>
              std::quoted(message.senderName) >> std::quoted(message.content) >>
              message.handled >> std::quoted(message.reply))) {
            return false;
        }
        loadedMessages.push_back(message);
    }

    personalUsers_ = std::move(loadedPersonalUsers);
    enterpriseUsers_ = std::move(loadedEnterpriseUsers);
    positions_ = std::move(loadedPositions);
    messages_ = std::move(loadedMessages);
    nextPersonalId_ = loadedNextPersonalId;
    nextEnterpriseId_ = loadedNextEnterpriseId;
    nextPositionId_ = loadedNextPositionId;
    nextMessageId_ = loadedNextMessageId;
    adminPassword_ = loadedAdminPassword;
    return true;
}

bool RecruitmentService::save() const {
    std::ofstream out(dataFilePath_, std::ios::out | std::ios::trunc);
    if (!out) {
        return false;
    }

    out << kDataFileMagic << '\n';
    out << nextPersonalId_ << ' ' << nextEnterpriseId_ << ' ' << nextPositionId_ << ' '
        << nextMessageId_ << ' ' << std::quoted(adminPassword_) << '\n';
    out << personalUsers_.size() << ' ' << enterpriseUsers_.size() << ' ' << positions_.size()
        << ' ' << messages_.size() << '\n';

    for (const auto& user : personalUsers_) {
        out << user.id << ' ' << std::quoted(user.username) << ' ' << std::quoted(user.password)
            << ' ' << std::quoted(user.name) << ' ' << std::quoted(user.gender) << ' '
            << std::quoted(user.phone) << ' ' << std::quoted(user.email) << ' '
            << std::quoted(user.resume.education) << ' '
            << std::quoted(user.resume.experience) << ' '
            << std::quoted(user.resume.skills) << ' '
            << std::quoted(user.resume.selfIntroduction) << ' '
            << static_cast<int>(user.status) << ' ' << user.appliedPositionIds.size();
        for (int positionId : user.appliedPositionIds) {
            out << ' ' << positionId;
        }
        out << '\n';
    }

    for (const auto& user : enterpriseUsers_) {
        out << user.id << ' ' << std::quoted(user.username) << ' ' << std::quoted(user.password)
            << ' ' << std::quoted(user.companyName) << ' '
            << std::quoted(user.contactPerson) << ' ' << std::quoted(user.phone) << ' '
            << std::quoted(user.email) << ' ' << std::quoted(user.address) << ' '
            << std::quoted(user.introduction) << ' ' << static_cast<int>(user.status) << '\n';
    }

    for (const auto& position : positions_) {
        out << position.id << ' ' << position.enterpriseId << ' '
            << std::quoted(position.title) << ' ' << std::quoted(position.location) << ' '
            << std::quoted(position.salary) << ' ' << std::quoted(position.requirement) << ' '
            << std::quoted(position.description) << ' ' << position.active << '\n';
    }

    for (const auto& message : messages_) {
        out << message.id << ' ' << std::quoted(message.senderType) << ' '
            << std::quoted(message.senderName) << ' ' << std::quoted(message.content) << ' '
            << message.handled << ' ' << std::quoted(message.reply) << '\n';
    }

    return static_cast<bool>(out);
}

bool RecruitmentService::persist(std::string& error) {
    if (!save()) {
        error = "保存数据失败。";
        return false;
    }
    return true;
}

void RecruitmentService::seedData() {
    personalUsers_.clear();
    enterpriseUsers_.clear();
    positions_.clear();
    messages_.clear();

    nextPersonalId_ = 1001;
    nextEnterpriseId_ = 2001;
    nextPositionId_ = 3001;
    nextMessageId_ = 4001;
    adminPassword_ = "admin123";

    PersonalUser personal;
    personal.id = nextPersonalId_++;
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
    personalUsers_.push_back(personal);

    PersonalUser pendingPersonal;
    pendingPersonal.id = nextPersonalId_++;
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
    personalUsers_.push_back(pendingPersonal);

    EnterpriseUser enterprise;
    enterprise.id = nextEnterpriseId_++;
    enterprise.username = "futurehr";
    enterprise.password = "123456";
    enterprise.companyName = "未来科技";
    enterprise.contactPerson = "何琳";
    enterprise.phone = "020-88886666";
    enterprise.email = "hr@futuretech.com";
    enterprise.address = "深圳";
    enterprise.introduction = "一家专注于人工智能与云产品研发的科技公司。";
    enterprise.status = AuditStatus::Approved;
    enterpriseUsers_.push_back(enterprise);

    EnterpriseUser pendingEnterprise;
    pendingEnterprise.id = nextEnterpriseId_++;
    pendingEnterprise.username = "greenhr";
    pendingEnterprise.password = "123456";
    pendingEnterprise.companyName = "绿源智造";
    pendingEnterprise.contactPerson = "陈杰";
    pendingEnterprise.phone = "0755-77889900";
    pendingEnterprise.email = "contact@greenstartup.com";
    pendingEnterprise.address = "广州";
    pendingEnterprise.introduction = "一家处于成长阶段的智能制造创业公司。";
    pendingEnterprise.status = AuditStatus::Pending;
    enterpriseUsers_.push_back(pendingEnterprise);

    Position p1;
    p1.id = nextPositionId_++;
    p1.enterpriseId = enterprise.id;
    p1.title = "C++ 开发工程师";
    p1.location = "深圳";
    p1.salary = "15k-25k";
    p1.requirement = "具备扎实的 C++ 基础，熟悉 STL";
    p1.description = "负责核心模块的开发与维护。";
    positions_.push_back(p1);

    Position p2;
    p2.id = nextPositionId_++;
    p2.enterpriseId = enterprise.id;
    p2.title = "后端工程师";
    p2.location = "深圳";
    p2.salary = "18k-30k";
    p2.requirement = "熟悉 Linux、数据库与网络基础";
    p2.description = "负责服务接口开发与后端性能优化。";
    positions_.push_back(p2);

    Message message;
    message.id = nextMessageId_++;
    message.senderType = "个人用户";
    message.senderName = "张琳";
    message.content = "希望系统中可以增加更多远程岗位。";
    messages_.push_back(message);
}

PersonalUser* RecruitmentService::findPersonalById(int id) {
    for (auto& user : personalUsers_) {
        if (user.id == id) {
            return &user;
        }
    }
    return nullptr;
}

PersonalUser* RecruitmentService::findPersonalByUsername(const std::string& username) {
    for (auto& user : personalUsers_) {
        if (user.username == username) {
            return &user;
        }
    }
    return nullptr;
}

EnterpriseUser* RecruitmentService::findEnterpriseById(int id) {
    for (auto& user : enterpriseUsers_) {
        if (user.id == id) {
            return &user;
        }
    }
    return nullptr;
}

EnterpriseUser* RecruitmentService::findEnterpriseByUsername(const std::string& username) {
    for (auto& user : enterpriseUsers_) {
        if (user.username == username) {
            return &user;
        }
    }
    return nullptr;
}

Position* RecruitmentService::findPositionById(int id) {
    for (auto& position : positions_) {
        if (position.id == id) {
            return &position;
        }
    }
    return nullptr;
}

Message* RecruitmentService::findMessageById(int id) {
    for (auto& message : messages_) {
        if (message.id == id) {
            return &message;
        }
    }
    return nullptr;
}

PersonalUser* RecruitmentService::loginPersonal(const std::string& username,
                                                const std::string& password) {
    PersonalUser* user = findPersonalByUsername(username);
    if (user == nullptr || user->password != password) {
        return nullptr;
    }
    return user;
}

EnterpriseUser* RecruitmentService::loginEnterprise(const std::string& username,
                                                    const std::string& password) {
    EnterpriseUser* user = findEnterpriseByUsername(username);
    if (user == nullptr || user->password != password) {
        return nullptr;
    }
    return user;
}

bool RecruitmentService::loginAdmin(const std::string& username,
                                    const std::string& password) const {
    return username == adminUsername_ && password == adminPassword_;
}

bool RecruitmentService::updatePersonalInfo(int personalId,
                                            const std::string& name,
                                            const std::string& gender,
                                            const std::string& phone,
                                            const std::string& email,
                                            std::string& error) {
    PersonalUser* user = findPersonalById(personalId);
    if (user == nullptr) {
        error = "未找到个人用户。";
        return false;
    }
    if (name.empty()) {
        error = "姓名不能为空。";
        return false;
    }

    user->name = name;
    user->gender = gender;
    user->phone = phone;
    user->email = email;
    return persist(error);
}

bool RecruitmentService::updatePersonalResume(int personalId,
                                              const Resume& resume,
                                              std::string& error) {
    PersonalUser* user = findPersonalById(personalId);
    if (user == nullptr) {
        error = "未找到个人用户。";
        return false;
    }

    user->resume = resume;
    return persist(error);
}

bool RecruitmentService::applyForPosition(int personalId, int positionId, std::string& error) {
    PersonalUser* user = findPersonalById(personalId);
    if (user == nullptr) {
        error = "未找到个人用户。";
        return false;
    }
    if (user->status != AuditStatus::Approved) {
        error = "当前账号尚未审核通过，暂时不能申请岗位。";
        return false;
    }

    Position* position = findPositionById(positionId);
    if (position == nullptr || !isPositionVisible(*position)) {
        error = "岗位不存在或当前不可申请。";
        return false;
    }

    if (std::find(user->appliedPositionIds.begin(), user->appliedPositionIds.end(), positionId) !=
        user->appliedPositionIds.end()) {
        error = "你已经申请过该岗位。";
        return false;
    }

    user->appliedPositionIds.push_back(positionId);
    return persist(error);
}

bool RecruitmentService::withdrawApplication(int personalId,
                                             int positionId,
                                             std::string& error) {
    PersonalUser* user = findPersonalById(personalId);
    if (user == nullptr) {
        error = "未找到个人用户。";
        return false;
    }

    auto it = std::find(user->appliedPositionIds.begin(), user->appliedPositionIds.end(), positionId);
    if (it == user->appliedPositionIds.end()) {
        error = "未找到该岗位的申请记录。";
        return false;
    }

    user->appliedPositionIds.erase(it);
    return persist(error);
}

bool RecruitmentService::changePersonalPassword(int personalId,
                                                const std::string& oldPassword,
                                                const std::string& newPassword,
                                                std::string& error) {
    PersonalUser* user = findPersonalById(personalId);
    if (user == nullptr) {
        error = "未找到个人用户。";
        return false;
    }
    if (user->password != oldPassword) {
        error = "当前密码不正确。";
        return false;
    }
    if (newPassword.empty()) {
        error = "新密码不能为空。";
        return false;
    }

    user->password = newPassword;
    return persist(error);
}

bool RecruitmentService::updateEnterpriseInfo(int enterpriseId,
                                              const std::string& companyName,
                                              const std::string& contactPerson,
                                              const std::string& phone,
                                              const std::string& email,
                                              const std::string& address,
                                              const std::string& introduction,
                                              std::string& error) {
    EnterpriseUser* user = findEnterpriseById(enterpriseId);
    if (user == nullptr) {
        error = "未找到企业用户。";
        return false;
    }
    if (companyName.empty()) {
        error = "企业名称不能为空。";
        return false;
    }

    user->companyName = companyName;
    user->contactPerson = contactPerson;
    user->phone = phone;
    user->email = email;
    user->address = address;
    user->introduction = introduction;
    return persist(error);
}

bool RecruitmentService::addPosition(int enterpriseId,
                                     const std::string& title,
                                     const std::string& location,
                                     const std::string& salary,
                                     const std::string& requirement,
                                     const std::string& description,
                                     std::string& error) {
    EnterpriseUser* user = findEnterpriseById(enterpriseId);
    if (user == nullptr) {
        error = "未找到企业用户。";
        return false;
    }
    if (user->status != AuditStatus::Approved) {
        error = "当前企业账号尚未审核通过，暂时不能发布岗位。";
        return false;
    }
    if (title.empty()) {
        error = "岗位名称不能为空。";
        return false;
    }

    Position position;
    position.id = nextPositionId_++;
    position.enterpriseId = enterpriseId;
    position.title = title;
    position.location = location;
    position.salary = salary;
    position.requirement = requirement;
    position.description = description;
    positions_.push_back(position);
    return persist(error);
}

bool RecruitmentService::updatePosition(int enterpriseId,
                                        int positionId,
                                        const std::string& title,
                                        const std::string& location,
                                        const std::string& salary,
                                        const std::string& requirement,
                                        const std::string& description,
                                        std::string& error) {
    EnterpriseUser* enterprise = findEnterpriseById(enterpriseId);
    if (enterprise == nullptr) {
        error = "未找到企业用户。";
        return false;
    }
    Position* position = findPositionById(positionId);
    if (position == nullptr || position->enterpriseId != enterpriseId || !position->active) {
        error = "未找到该岗位。";
        return false;
    }
    if (title.empty()) {
        error = "岗位名称不能为空。";
        return false;
    }

    position->title = title;
    position->location = location;
    position->salary = salary;
    position->requirement = requirement;
    position->description = description;
    return persist(error);
}

bool RecruitmentService::removePosition(int enterpriseId, int positionId, std::string& error) {
    EnterpriseUser* enterprise = findEnterpriseById(enterpriseId);
    if (enterprise == nullptr) {
        error = "未找到企业用户。";
        return false;
    }
    Position* position = findPositionById(positionId);
    if (position == nullptr || position->enterpriseId != enterpriseId || !position->active) {
        error = "未找到该岗位。";
        return false;
    }

    position->active = false;
    return persist(error);
}

bool RecruitmentService::changeEnterprisePassword(int enterpriseId,
                                                  const std::string& oldPassword,
                                                  const std::string& newPassword,
                                                  std::string& error) {
    EnterpriseUser* user = findEnterpriseById(enterpriseId);
    if (user == nullptr) {
        error = "未找到企业用户。";
        return false;
    }
    if (user->password != oldPassword) {
        error = "当前密码不正确。";
        return false;
    }
    if (newPassword.empty()) {
        error = "新密码不能为空。";
        return false;
    }

    user->password = newPassword;
    return persist(error);
}

bool RecruitmentService::addMessage(const std::string& senderType,
                                    const std::string& senderName,
                                    const std::string& content,
                                    std::string& error) {
    if (content.empty()) {
        error = "留言内容不能为空。";
        return false;
    }

    Message message;
    message.id = nextMessageId_++;
    message.senderType = senderType;
    message.senderName = senderName;
    message.content = content;
    messages_.push_back(message);
    return persist(error);
}

bool RecruitmentService::reviewPersonalUser(int userId, AuditStatus status, std::string& error) {
    PersonalUser* user = findPersonalById(userId);
    if (user == nullptr) {
        error = "未找到个人用户。";
        return false;
    }

    user->status = status;
    return persist(error);
}

bool RecruitmentService::reviewEnterpriseUser(int userId,
                                              AuditStatus status,
                                              std::string& error) {
    EnterpriseUser* user = findEnterpriseById(userId);
    if (user == nullptr) {
        error = "未找到企业用户。";
        return false;
    }

    user->status = status;
    return persist(error);
}

bool RecruitmentService::replyMessage(int messageId,
                                      const std::string& reply,
                                      std::string& error) {
    Message* message = findMessageById(messageId);
    if (message == nullptr) {
        error = "未找到该留言。";
        return false;
    }
    if (reply.empty()) {
        error = "回复内容不能为空。";
        return false;
    }

    message->reply = reply;
    message->handled = true;
    return persist(error);
}

bool RecruitmentService::deleteMessage(int messageId, std::string& error) {
    auto it = std::find_if(messages_.begin(), messages_.end(),
                           [messageId](const Message& message) { return message.id == messageId; });
    if (it == messages_.end()) {
        error = "未找到该留言。";
        return false;
    }

    messages_.erase(it);
    return persist(error);
}

bool RecruitmentService::changeAdminPassword(const std::string& oldPassword,
                                             const std::string& newPassword,
                                             std::string& error) {
    if (adminPassword_ != oldPassword) {
        error = "当前密码不正确。";
        return false;
    }
    if (newPassword.empty()) {
        error = "新密码不能为空。";
        return false;
    }

    adminPassword_ = newPassword;
    return persist(error);
}

const PersonalUser* RecruitmentService::personalById(int id) const {
    for (const auto& user : personalUsers_) {
        if (user.id == id) {
            return &user;
        }
    }
    return nullptr;
}

const EnterpriseUser* RecruitmentService::enterpriseById(int id) const {
    for (const auto& user : enterpriseUsers_) {
        if (user.id == id) {
            return &user;
        }
    }
    return nullptr;
}

const Position* RecruitmentService::positionById(int id) const {
    for (const auto& position : positions_) {
        if (position.id == id) {
            return &position;
        }
    }
    return nullptr;
}

const std::vector<PersonalUser>& RecruitmentService::personalUsers() const {
    return personalUsers_;
}

const std::vector<EnterpriseUser>& RecruitmentService::enterpriseUsers() const {
    return enterpriseUsers_;
}

const std::vector<Position>& RecruitmentService::positions() const {
    return positions_;
}

const std::vector<Message>& RecruitmentService::messages() const {
    return messages_;
}

bool RecruitmentService::isPositionVisible(const Position& position) const {
    const EnterpriseUser* enterprise = enterpriseById(position.enterpriseId);
    return position.active && enterprise != nullptr && enterprise->status == AuditStatus::Approved;
}

bool RecruitmentService::usernameExists(const std::string& username) const {
    if (username == adminUsername_) {
        return true;
    }

    const auto personalIt =
        std::find_if(personalUsers_.begin(), personalUsers_.end(),
                     [&username](const PersonalUser& user) { return user.username == username; });
    if (personalIt != personalUsers_.end()) {
        return true;
    }

    const auto enterpriseIt = std::find_if(
        enterpriseUsers_.begin(), enterpriseUsers_.end(),
        [&username](const EnterpriseUser& user) { return user.username == username; });
    return enterpriseIt != enterpriseUsers_.end();
}

std::vector<const Position*> RecruitmentService::visiblePositions() const {
    std::vector<const Position*> result;
    for (const auto& position : positions_) {
        if (isPositionVisible(position)) {
            result.push_back(&position);
        }
    }
    return result;
}

std::vector<const Position*> RecruitmentService::positionsForEnterprise(int enterpriseId) const {
    std::vector<const Position*> result;
    for (const auto& position : positions_) {
        if (position.enterpriseId == enterpriseId && position.active) {
            result.push_back(&position);
        }
    }
    return result;
}

std::vector<const Position*> RecruitmentService::appliedPositionsForPersonal(int personalId) const {
    std::vector<const Position*> result;
    const PersonalUser* user = personalById(personalId);
    if (user == nullptr) {
        return result;
    }

    for (int positionId : user->appliedPositionIds) {
        const Position* position = positionById(positionId);
        if (position != nullptr) {
            result.push_back(position);
        }
    }
    return result;
}

std::vector<const PersonalUser*> RecruitmentService::approvedTalents() const {
    std::vector<const PersonalUser*> result;
    for (const auto& user : personalUsers_) {
        if (user.status == AuditStatus::Approved) {
            result.push_back(&user);
        }
    }
    return result;
}

std::vector<const PersonalUser*> RecruitmentService::pendingPersonalUsers() const {
    std::vector<const PersonalUser*> result;
    for (const auto& user : personalUsers_) {
        if (user.status == AuditStatus::Pending) {
            result.push_back(&user);
        }
    }
    return result;
}

std::vector<const EnterpriseUser*> RecruitmentService::pendingEnterpriseUsers() const {
    std::vector<const EnterpriseUser*> result;
    for (const auto& user : enterpriseUsers_) {
        if (user.status == AuditStatus::Pending) {
            result.push_back(&user);
        }
    }
    return result;
}

std::string RecruitmentService::adminUsername() const {
    return adminUsername_;
}

std::string RecruitmentService::adminPassword() const {
    return adminPassword_;
}

std::string RecruitmentService::statusText(AuditStatus status) {
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
