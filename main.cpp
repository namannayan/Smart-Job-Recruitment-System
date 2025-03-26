#include <iostream>
#include <mysql.h>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <cctype>
#include <sstream>
#include <iomanip>

using namespace std;

// Configuration (should be externalized in production)
const char DB_HOST[] = "localhost";
const char DB_USER[] = "root";
const char DB_PASS[] = "123456789";
const char DB_NAME[] = "JobRecruitment";

// Secure Database Connection Class
class Database {
private:
    MYSQL* conn;
    
    void initializeDatabase() {
        // Create database if it doesn't exist
        const char* createDB = "CREATE DATABASE IF NOT EXISTS JobRecruitment";
        if (mysql_query(conn, createDB) != 0) {
            throw runtime_error("Failed to create database: " + string(mysql_error(conn)));
        }
        
        // Select the database
        if (mysql_select_db(conn, DB_NAME) != 0) {
            throw runtime_error("Failed to select database: " + string(mysql_error(conn)));
        }
        
        // Create tables
        const char* createUsersTable = 
            "CREATE TABLE IF NOT EXISTS Users ("
            "user_id INT AUTO_INCREMENT PRIMARY KEY, "
            "name VARCHAR(100) NOT NULL, "
            "email VARCHAR(100) UNIQUE NOT NULL, "
            "password VARCHAR(255) NOT NULL, "
            "role ENUM('JobSeeker', 'Recruiter') NOT NULL, "
            "resume TEXT)";
            
        const char* createJobsTable = 
            "CREATE TABLE IF NOT EXISTS Jobs ("
            "job_id INT AUTO_INCREMENT PRIMARY KEY, "
            "recruiter_id INT NOT NULL, "
            "title VARCHAR(100) NOT NULL, "
            "description TEXT NOT NULL, "
            "location VARCHAR(100) NOT NULL, "
            "skills_required TEXT NOT NULL, "
            "salary INT NOT NULL, "
            "FOREIGN KEY (recruiter_id) REFERENCES Users(user_id))";

        if (mysql_query(conn, createUsersTable) != 0) {
            throw runtime_error("Failed to create Users table: " + string(mysql_error(conn)));
        }
        
        if (mysql_query(conn, createJobsTable) != 0) {
            throw runtime_error("Failed to create Jobs table: " + string(mysql_error(conn)));
        }
    }

public:
    Database() : conn(mysql_init(nullptr)) {
        if (!conn) throw runtime_error("MySQL initialization failed");
        
        // First connect without specifying a database
        if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, NULL, 3306, nullptr, 0)) {
            string err = "Connection failed: ";
            err += mysql_error(conn);
            mysql_close(conn);
            throw runtime_error(err);
        }
        
        // Now initialize the database and tables
        initializeDatabase();
        cout << "Database connected successfully\n";
    }

    ~Database() {
        if (conn) mysql_close(conn);
    }

    MYSQL* getConnection() const { return conn; }

    bool executeQuery(const string& query) {
        if (mysql_query(conn, query.c_str()) != 0) {
            cerr << "Query failed: " << mysql_error(conn) << "\n";
            return false;
        }
        return true;
    }

    MYSQL_RES* executeQueryWithResult(const string& query) {
        if (!executeQuery(query)) return nullptr;
        return mysql_store_result(conn);
    }
};

// Utility Functions
namespace Utils {
    string trim(const string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (string::npos == first) return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, (last - first + 1));
    }

    string toLower(const string& str) {
        string lowerStr = str;
        transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
        return lowerStr;
    }

    string hashPassword(const string& password) {
        // In production, use proper hashing like bcrypt
        // This is just a placeholder - replace with actual secure hashing
        return password;
    }

    vector<string> tokenize(const string& text) {
        vector<string> words;
        string word;
        stringstream ss(text);
        
        while (ss >> word) {
            word.erase(remove_if(word.begin(), word.end(), ::ispunct), word.end());
            words.push_back(toLower(word));
        }
        
        return words;
    }

    void clearScreen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }

    void pause() {
        cout << "\nPress Enter to continue...";
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
};

// User Management
class UserManager {
protected:
    int user_id;
    string name;
    string email;
    string role;
    string resume;
    bool logged_in;

public:
    UserManager() : user_id(-1), logged_in(false) {}

    bool registerUser(Database& db, const string& name, const string& email, 
                     const string& password, const string& role, const string& resume = "") {
        if (role != "JobSeeker" && role != "Recruiter") {
            cerr << "Invalid role. Must be 'JobSeeker' or 'Recruiter'.\n";
            return false;
        }

        string hashed_pwd = Utils::hashPassword(password);
        string query = "INSERT INTO Users (name, email, password, role, resume) VALUES ('" +
                       Utils::trim(name) + "', '" + Utils::trim(email) + "', '" + 
                       hashed_pwd + "', '" + role + "', '" + Utils::trim(resume) + "')";

        if (!db.executeQuery(query)) {
            return false;
        }
        
        cout << "Registration successful\n";
        return true;
    }

    bool login(Database& db, const string& email, const string& password) {
        string hashed_pwd = Utils::hashPassword(password);
        string query = "SELECT user_id, name, role, resume FROM Users WHERE email='" + 
                       Utils::trim(email) + "' AND password='" + hashed_pwd + "'";

        MYSQL_RES* res = db.executeQueryWithResult(query);
        if (!res) return false;

        MYSQL_ROW row = mysql_fetch_row(res);
        if (row) {
            user_id = atoi(row[0]);
            name = row[1];
            role = row[2];
            resume = row[3] ? row[3] : "";
            logged_in = true;
            cout << "Welcome, " << name << " (" << role << ")\n";
            mysql_free_result(res);
            return true;
        }

        mysql_free_result(res);
        cerr << "Invalid credentials\n";
        return false;
    }

    void updateResume(Database& db, const string& newResume) {
        if (!logged_in) {
            cerr << "You must be logged in to update your resume\n";
            return;
        }

        string query = "UPDATE Users SET resume = '" + Utils::trim(newResume) + 
                      "' WHERE user_id = " + to_string(user_id);
        
        if (db.executeQuery(query)) {
            resume = newResume;
            cout << "Resume updated successfully\n";
        }
    }

    bool isLoggedIn() const { return logged_in; }
    string getRole() const { return role; }
    int getUserId() const { return user_id; }
    string getName() const { return name; }
    string getResume() const { return resume; }
};

// Job Management
class JobManager {
private:
    Database& db;

public:
    JobManager(Database& database) : db(database) {}

    bool postJob(int recruiter_id, const string& title, const string& description, 
                const string& location, const string& skills, int salary) {
        string query = "INSERT INTO Jobs (recruiter_id, title, description, location, skills_required, salary) VALUES (" +
                      to_string(recruiter_id) + ", '" + Utils::trim(title) + "', '" + 
                      Utils::trim(description) + "', '" + Utils::trim(location) + "', '" + 
                      Utils::trim(skills) + "', " + to_string(salary) + ")";
        
        return db.executeQuery(query);
    }

    void displayAllJobs() {
        string query = "SELECT j.job_id, j.title, j.location, j.salary, u.name FROM Jobs j "
                       "JOIN Users u ON j.recruiter_id = u.user_id";
        
        MYSQL_RES* res = db.executeQueryWithResult(query);
        if (!res) return;

        cout << "\nAvailable Jobs:\n";
        cout << "-----------------------------------------------------------------\n";
        cout << left << setw(5) << "ID" << setw(25) << "Title" << setw(20) << "Location" 
             << setw(10) << "Salary" << setw(20) << "Recruiter" << "\n";
        cout << "-----------------------------------------------------------------\n";

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            cout << left << setw(5) << row[0] << setw(25) << (strlen(row[1]) > 20 ? string(row[1]).substr(0, 17) + "..." : row[1])
                 << setw(20) << row[2] << setw(10) << row[3] << setw(20) << row[4] << "\n";
        }
        cout << "-----------------------------------------------------------------\n";
        mysql_free_result(res);
    }
};

// Job Search using Trie
class JobSearchEngine {
private:
    struct TrieNode {
        TrieNode* children[26] = {nullptr};
        vector<int> jobIds;
        
        ~TrieNode() {
            for (int i = 0; i < 26; i++) {
                delete children[i];
            }
        }
    };

    TrieNode* root;
    Database& db;

    // Private methods
    void clearTrie(TrieNode* node) {
        if (!node) return;
        for (int i = 0; i < 26; i++) {
            clearTrie(node->children[i]);
        }
        delete node;
    }

    void insertJob(const string& title, int jobId) {
        TrieNode* node = root;
        for (char ch : title) {
            ch = tolower(ch);
            if (ch < 'a' || ch > 'z') continue;
            
            if (!node->children[ch - 'a']) {
                node->children[ch - 'a'] = new TrieNode();
            }
            node = node->children[ch - 'a'];
        }
        node->jobIds.push_back(jobId);
    }

public:
    // Constructor
    explicit JobSearchEngine(Database& database) : db(database), root(new TrieNode()) {
        loadJobsFromDatabase();
    }

    // Delete copy constructor and assignment operator
    JobSearchEngine(const JobSearchEngine&) = delete;
    JobSearchEngine& operator=(const JobSearchEngine&) = delete;

    // Move constructor
    JobSearchEngine(JobSearchEngine&& other) noexcept 
        : db(other.db), root(other.root) {
        other.root = nullptr;
    }

    // Move assignment operator
    JobSearchEngine& operator=(JobSearchEngine&& other) noexcept {
        if (this != &other) {
            clearTrie(root);
            root = other.root;
            other.root = nullptr;
        }
        return *this;
    }

    // Destructor
    ~JobSearchEngine() {
        clearTrie(root);
    }

    // Public interface
    void loadJobsFromDatabase() {
        clearTrie(root);
        root = new TrieNode();
        
        string query = "SELECT job_id, title FROM Jobs";
        MYSQL_RES* res = db.executeQueryWithResult(query);
        if (!res) return;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            int jobId = atoi(row[0]);
            string title = row[1];
            insertJob(title, jobId);
        }
        mysql_free_result(res);
    }

    void searchJobs(const string& prefix) {
        TrieNode* node = root;
        for (char ch : prefix) {
            ch = tolower(ch);
            if (ch < 'a' || ch > 'z') continue;
            
            if (!node->children[ch - 'a']) {
                cout << "No jobs found with this prefix\n";
                return;
            }
            node = node->children[ch - 'a'];
        }

        if (node->jobIds.empty()) {
            cout << "No jobs found with this prefix\n";
            return;
        }

        cout << "\nMatching Jobs:\n";
        cout << "----------------------------------------\n";
        for (int jobId : node->jobIds) {
            string query = "SELECT j.title, j.location, j.salary, u.name FROM Jobs j "
                          "JOIN Users u ON j.recruiter_id = u.user_id "
                          "WHERE j.job_id = " + to_string(jobId);
            
            MYSQL_RES* res = db.executeQueryWithResult(query);
            if (res) {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row) {
                    cout << "ID: " << jobId << " | Title: " << row[0] 
                         << " | Location: " << row[1] 
                         << " | Salary: " << row[2] 
                         << " | Recruiter: " << row[3] << "\n";
                }
                mysql_free_result(res);
            }
        }
        cout << "----------------------------------------\n";
    }
};

// Job Matching Algorithm
class JobMatcher {
private:
    Database& db;

    map<string, double> calculateTF(const vector<string>& words) {
        map<string, double> tf;
        int totalWords = words.size();
        
        for (const string& word : words) {
            tf[word]++;
        }
        
        // Normalize TF values
        for (auto &entry : tf) {
            entry.second /= totalWords;
        }

        return tf;
    }

    map<string, double> calculateIDF(const vector<vector<string>>& allResumes) {
        map<string, double> idf;
        int totalDocuments = allResumes.size();
        
        for (const auto& resume : allResumes) {
            map<string, bool> seen;
            for (const string& word : resume) {
                if (!seen[word]) {
                    idf[word]++;
                    seen[word] = true;
                }
            }
        }

        // Apply IDF formula with smoothing
        for (auto &entry : idf) {
            entry.second = log((totalDocuments + 1.0) / (1.0 + entry.second)) + 1;
        }

        return idf;
    }

    double calculateTFIDFScore(const map<string, double>& tf, 
                             const map<string, double>& idf, 
                             const vector<string>& jobDescription) {
        double score = 0.0;
        for (const string& word : jobDescription) {
            auto tfIt = tf.find(word);
            auto idfIt = idf.find(word);
            
            if (tfIt != tf.end() && idfIt != idf.end()) {
                score += tfIt->second * idfIt->second;
            }
        }
        return score;
    }

public:
    JobMatcher(Database& database) : db(database) {}

    void rankResumes(const string& jobDescription) {
        vector<vector<string>> allResumes;
        vector<string> jobWords = Utils::tokenize(jobDescription);
        vector<pair<int, double>> rankedResumes;

        // Fetch all resumes from the database
        string query = "SELECT user_id, name, resume FROM Users WHERE role='JobSeeker' AND resume IS NOT NULL";
        MYSQL_RES* res = db.executeQueryWithResult(query);
        if (!res) {
            cerr << "No resumes found in the database\n";
            return;
        }

        map<int, map<string, double>> tfData;
        map<int, string> userNames;
        map<int, vector<string>> resumeWords;
        
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            int userId = atoi(row[0]);
            string name = row[1];
            string resume = row[2];

            vector<string> words = Utils::tokenize(resume);
            allResumes.push_back(words);
            resumeWords[userId] = words;
            tfData[userId] = calculateTF(words);
            userNames[userId] = name;
        }
        mysql_free_result(res);

        if (allResumes.empty()) {
            cout << "No resumes found in the database\n";
            return;
        }

        // Calculate IDF using all resumes
        map<string, double> idf = calculateIDF(allResumes);

        // Compute TF-IDF scores for each resume
        for (auto &entry : tfData) {
            int userId = entry.first;
            double score = calculateTFIDFScore(entry.second, idf, jobWords);
            rankedResumes.push_back({userId, score});
        }

        // Sort resumes by highest TF-IDF score
        sort(rankedResumes.begin(), rankedResumes.end(), 
            [](const pair<int, double>& a, const pair<int, double>& b) {
                return a.second > b.second;
            });

        // Display ranked results
        cout << "\nTop Matching Candidates:\n";
        cout << "-----------------------------------------------------------------\n";
        cout << left << setw(5) << "Rank" << setw(20) << "Candidate" << setw(15) << "Score" << "Resume Excerpt\n";
        cout << "-----------------------------------------------------------------\n";
        
        for (size_t i = 0; i < rankedResumes.size() && i < 10; ++i) {
            int userId = rankedResumes[i].first;
            string excerpt = resumeWords[userId].size() > 5 ? 
                resumeWords[userId][0] + " " + resumeWords[userId][1] + "..." : "";
            
            cout << left << setw(5) << i+1 
                 << setw(20) << userNames[userId] 
                 << setw(15) << fixed << setprecision(3) << rankedResumes[i].second
                 << excerpt << "\n";
        }
        cout << "-----------------------------------------------------------------\n";
    }
};

// Main Application
class JobRecruitmentSystem {
private:
    Database db;
    UserManager userManager;
    JobManager jobManager;
    JobSearchEngine jobSearch;
    JobMatcher jobMatcher;

    void showMainMenu() {
        Utils::clearScreen();
        cout << "========================================\n";
        cout << "      JOB RECRUITMENT SYSTEM\n";
        cout << "========================================\n";
        
        if (userManager.isLoggedIn()) {
            cout << "Logged in as: " << userManager.getName() << " (" << userManager.getRole() << ")\n";
            cout << "----------------------------------------\n";
        }
        
        // Only show Register/Login when not logged in
        if (!userManager.isLoggedIn()) {
            cout << "1. Register\n";
            cout << "2. Login\n";
        }
        
        if (userManager.isLoggedIn()) {
            if (userManager.getRole() == "Recruiter") {
                cout << "3. Post Job\n";
                cout << "4. View All Jobs\n";
                cout << "5. Search Candidates\n";
            } else {
                cout << "3. Update Resume\n";
                cout << "4. Search Jobs\n";
                cout << "5. View All Jobs\n";
            }
            cout << "6. Logout\n";
        } else {
            // These options are available to everyone (logged in or not)
            cout << "3. View All Jobs\n";
            cout << "4. Search Jobs\n";
        }
        
        cout << "0. Exit\n";
        cout << "----------------------------------------\n";
        cout << "Enter your choice: ";
    }

    void handleRegistration() {
        Utils::clearScreen();
        cout << "REGISTRATION\n";
        cout << "----------------------------------------\n";
        
        string name, email, password, role, resume;
        cout << "Name: ";
        getline(cin, name);
        cout << "Email: ";
        getline(cin, email);
        cout << "Password: ";
        getline(cin, password);
        cout << "Role (JobSeeker/Recruiter): ";
        getline(cin, role);
        
        if (role == "JobSeeker") {
            cout << "Resume Text: ";
            getline(cin, resume);
        }

        if (userManager.registerUser(db, name, email, password, role, resume)) {
            cout << "\nRegistration successful!\n";
        } else {
            cout << "\nRegistration failed. Please try again.\n";
        }
        Utils::pause();
    }

    void handleLogin() {
        Utils::clearScreen();
        cout << "LOGIN\n";
        cout << "----------------------------------------\n";
        
        string email, password;
        cout << "Email: ";
        getline(cin, email);
        cout << "Password: ";
        getline(cin, password);
        
        if (userManager.login(db, email, password)) {
            cout << "\nLogin successful!\n";
        } else {
            cout << "\nLogin failed. Invalid credentials.\n";
        }
        Utils::pause();
    }

    void handleJobPosting() {
        if (!userManager.isLoggedIn() || userManager.getRole() != "Recruiter") {
            cout << "Unauthorized action\n";
            Utils::pause();
            return;
        }

        Utils::clearScreen();
        cout << "POST A JOB\n";
        cout << "----------------------------------------\n";
        
        string title, description, location, skills;
        int salary;
        
        cout << "Job Title: ";
        getline(cin, title);
        cout << "Description: ";
        getline(cin, description);
        cout << "Location: ";
        getline(cin, location);
        cout << "Required Skills: ";
        getline(cin, skills);
        cout << "Salary: ";
        cin >> salary;
        cin.ignore();
        
        if (jobManager.postJob(userManager.getUserId(), title, description, location, skills, salary)) {
            cout << "\nJob posted successfully!\n";
            jobSearch = JobSearchEngine(db); // Refresh search index
        } else {
            cout << "\nFailed to post job. Please try again.\n";
        }
        Utils::pause();
    }

    void handleResumeUpdate() {
        if (!userManager.isLoggedIn() || userManager.getRole() != "JobSeeker") {
            cout << "Unauthorized action\n";
            Utils::pause();
            return;
        }

        Utils::clearScreen();
        cout << "UPDATE RESUME\n";
        cout << "----------------------------------------\n";
        cout << "Current Resume:\n" << userManager.getResume() << "\n";
        cout << "----------------------------------------\n";
        
        string newResume;
        cout << "Enter new resume text:\n";
        getline(cin, newResume);
        
        userManager.updateResume(db, newResume);
        Utils::pause();
    }

    void handleJobSearch() {
        Utils::clearScreen();
        cout << "JOB SEARCH\n";
        cout << "----------------------------------------\n";
        
        string keyword;
        cout << "Enter job title keyword: ";
        getline(cin, keyword);
        
        jobSearch.searchJobs(keyword);
        Utils::pause();
    }

    void handleCandidateSearch() {
        if (!userManager.isLoggedIn() || userManager.getRole() != "Recruiter") {
            cout << "Unauthorized action\n";
            Utils::pause();
            return;
        }

        Utils::clearScreen();
        cout << "FIND CANDIDATES\n";
        cout << "----------------------------------------\n";
        
        string jobDescription;
        cout << "Enter job description to match candidates:\n";
        getline(cin, jobDescription);
        
        jobMatcher.rankResumes(jobDescription);
        Utils::pause();
    }

public:
    JobRecruitmentSystem() : 
        jobManager(db), 
        jobSearch(db), 
        jobMatcher(db) {}

    void run() {
        int choice;
        do {
            showMainMenu();
            cin >> choice;
            cin.ignore();
            
            switch (choice) {
                case 1: 
                    if (!userManager.isLoggedIn()) {
                        handleRegistration();
                    } else {
                        cout << "Invalid choice. Please try again.\n";
                        Utils::pause();
                    }
                    break;
                case 2: 
                    if (!userManager.isLoggedIn()) {
                        handleLogin();
                    } else {
                        cout << "Invalid choice. Please try again.\n";
                        Utils::pause();
                    }
                    break;
                case 3: 
                    if (userManager.isLoggedIn()) {
                        if (userManager.getRole() == "Recruiter") {
                            handleJobPosting();
                        } else {
                            handleResumeUpdate();
                        }
                    } else {
                        jobManager.displayAllJobs();
                        Utils::pause();
                    }
                    break;
                case 4:
                    if (userManager.isLoggedIn()) {
                        if (userManager.getRole() == "Recruiter") {
                            jobManager.displayAllJobs();
                            Utils::pause();
                        } else {
                            handleJobSearch();
                        }
                    } else {
                        handleJobSearch();
                    }
                    break;
                case 5:
                    if (userManager.isLoggedIn()) {
                        if (userManager.getRole() == "Recruiter") {
                            handleCandidateSearch();
                        } else {
                            jobManager.displayAllJobs();
                            Utils::pause();
                        }
                    } else {
                        cout << "Invalid choice. Please try again.\n";
                        Utils::pause();
                    }
                    break;
                case 6:
                    if (userManager.isLoggedIn()) {
                        userManager = UserManager(); // Reset user
                        cout << "Logged out successfully\n";
                        Utils::pause();
                    } else {
                        cout << "Invalid choice. Please try again.\n";
                        Utils::pause();
                    }
                    break;
                case 0:
                    cout << "Exiting system...\n";
                    break;
                default:
                    cout << "Invalid choice. Please try again.\n";
                    Utils::pause();
            }
        } while (choice != 0);
    }
};

int main() {
    try {
        JobRecruitmentSystem system;
        system.run();
    } catch (const exception& e) {
        cerr << "System error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
