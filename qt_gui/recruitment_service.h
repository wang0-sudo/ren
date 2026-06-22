#pragma once

#include <string>
#include <vector>

enum class AuditStatus { Pending, Approved, Rejected };
enum class UserRole { Personal, Enterprise, Admin };

struct Resume {
    std::string education;
    std::string experience;
    std::string skills;
    std::string selfIntroduction;
};

struct PersonalUser {
    int id = 0;
    std::string username;
    std::string password;
    std::string name;
    std::string gender;
    std::string phone;
    std::string email;
    Resume resume;
    std::vector<int> appliedPositionIds;
    AuditStatus status = AuditStatus::Pending;
};

struct EnterpriseUser {
    int id = 0;
    std::string username;
    std::string password;
    std::string companyName;
    std::string contactPerson;
    std::string phone;
    std::string email;
    std::string address;
    std::string introduction;
    AuditStatus status = AuditStatus::Pending;
};

struct Position {
    int id = 0;
    int enterpriseId = 0;
    std::string title;
    std::string location;
    std::string salary;
    std::string requirement;
    std::string description;
    bool active = true;
};

struct Message {
    int id = 0;
    std::string senderType;
    std::string senderName;
    std::string content;
    bool handled = false;
    std::string reply;
};

class RecruitmentService {
public:
    explicit RecruitmentService(std::string dataFilePath = "recruitment_data.txt");

    void loadOrSeed();
    bool load();
    bool save() const;

    bool registerPersonalUser(const std::string& username,
                              const std::string& password,
                              const std::string& name,
                              const std::string& gender,
                              const std::string& phone,
                              const std::string& email,
                              const Resume& resume,
                              std::string& error);
    bool registerEnterpriseUser(const std::string& username,
                                const std::string& password,
                                const std::string& companyName,
                                const std::string& contactPerson,
                                const std::string& phone,
                                const std::string& email,
                                const std::string& address,
                                const std::string& introduction,
                                std::string& error);

    PersonalUser* loginPersonal(const std::string& username, const std::string& password);
    EnterpriseUser* loginEnterprise(const std::string& username, const std::string& password);
    bool loginAdmin(const std::string& username, const std::string& password) const;

    bool updatePersonalInfo(int personalId,
                            const std::string& name,
                            const std::string& gender,
                            const std::string& phone,
                            const std::string& email,
                            std::string& error);
    bool updatePersonalResume(int personalId, const Resume& resume, std::string& error);
    bool applyForPosition(int personalId, int positionId, std::string& error);
    bool withdrawApplication(int personalId, int positionId, std::string& error);
    bool changePersonalPassword(int personalId,
                                const std::string& oldPassword,
                                const std::string& newPassword,
                                std::string& error);

    bool updateEnterpriseInfo(int enterpriseId,
                              const std::string& companyName,
                              const std::string& contactPerson,
                              const std::string& phone,
                              const std::string& email,
                              const std::string& address,
                              const std::string& introduction,
                              std::string& error);
    bool addPosition(int enterpriseId,
                     const std::string& title,
                     const std::string& location,
                     const std::string& salary,
                     const std::string& requirement,
                     const std::string& description,
                     std::string& error);
    bool updatePosition(int enterpriseId,
                        int positionId,
                        const std::string& title,
                        const std::string& location,
                        const std::string& salary,
                        const std::string& requirement,
                        const std::string& description,
                        std::string& error);
    bool removePosition(int enterpriseId, int positionId, std::string& error);
    bool changeEnterprisePassword(int enterpriseId,
                                  const std::string& oldPassword,
                                  const std::string& newPassword,
                                  std::string& error);

    bool addMessage(const std::string& senderType,
                    const std::string& senderName,
                    const std::string& content,
                    std::string& error);
    bool reviewPersonalUser(int userId, AuditStatus status, std::string& error);
    bool reviewEnterpriseUser(int userId, AuditStatus status, std::string& error);
    bool replyMessage(int messageId, const std::string& reply, std::string& error);
    bool deleteMessage(int messageId, std::string& error);
    bool changeAdminPassword(const std::string& oldPassword,
                             const std::string& newPassword,
                             std::string& error);

    const PersonalUser* personalById(int id) const;
    const EnterpriseUser* enterpriseById(int id) const;
    const Position* positionById(int id) const;

    const std::vector<PersonalUser>& personalUsers() const;
    const std::vector<EnterpriseUser>& enterpriseUsers() const;
    const std::vector<Position>& positions() const;
    const std::vector<Message>& messages() const;

    std::vector<const Position*> visiblePositions() const;
    std::vector<const Position*> positionsForEnterprise(int enterpriseId) const;
    std::vector<const Position*> appliedPositionsForPersonal(int personalId) const;
    std::vector<const PersonalUser*> approvedTalents() const;
    std::vector<const PersonalUser*> pendingPersonalUsers() const;
    std::vector<const EnterpriseUser*> pendingEnterpriseUsers() const;

    std::string adminUsername() const;
    std::string adminPassword() const;
    static std::string statusText(AuditStatus status);

private:
    void seedData();
    bool persist(std::string& error);
    bool isPositionVisible(const Position& position) const;
    bool usernameExists(const std::string& username) const;
    PersonalUser* findPersonalById(int id);
    PersonalUser* findPersonalByUsername(const std::string& username);
    EnterpriseUser* findEnterpriseById(int id);
    EnterpriseUser* findEnterpriseByUsername(const std::string& username);
    Position* findPositionById(int id);
    Message* findMessageById(int id);

    std::vector<PersonalUser> personalUsers_;
    std::vector<EnterpriseUser> enterpriseUsers_;
    std::vector<Position> positions_;
    std::vector<Message> messages_;

    std::string adminUsername_ = "admin";
    std::string adminPassword_ = "admin123";
    std::string dataFilePath_;

    int nextPersonalId_ = 1001;
    int nextEnterpriseId_ = 2001;
    int nextPositionId_ = 3001;
    int nextMessageId_ = 4001;
};
