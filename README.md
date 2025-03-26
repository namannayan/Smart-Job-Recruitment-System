Smart Job Recruitment System

Overview

The Smart Job Recruitment System is a C++ application that facilitates job seekers and recruiters in managing job postings and applications. It utilizes MySQL for database management and provides functionalities such as user registration, job posting, resume updates, job search, and candidate ranking.

Features:

(i) User Authentication: Register and log in as a job seeker or recruiter.

(ii) Database Management: Secure connection to MySQL with automatic table creation.

(iii) Job Posting: Recruiters can post job opportunities with relevant details.

(iv) Resume Upload: Job seekers can update their resumes.

(v) Job Search: Users can search for jobs based on keywords.

(vi)Candidate Ranking: Recruiters can match candidates to job descriptions using TF-IDF scoring.

(vii)Optimized Search: Trie-based search for efficient job searching.




Technologies Used:

(i)Programming Language: C++

(ii)Database: MySQL

(iii)Libraries:

(iv)MySQL Connector/C++

(v)Standard Template Library (STL)


Setup Instructions:

Prerequisites

Install MySQL Server

Install MySQL Connector/C++

Ensure g++ is installed for compilation

Steps to Run

Clone the repository:

git clone https://github.com/yourusername/job-recruitment-system.git
cd job-recruitment-system

Compile the project:

g++ main.cpp -o job_recruitment -lmysqlclient

Run the application:

./job_recruitment

Database Configuration

Modify the database connection credentials in main.cpp if needed:

const char DB_HOST[] = "localhost";

const char DB_USER[] = "root";

const char DB_PASS[] = "yourpassword";

const char DB_NAME[] = "JobRecruitment";


Contributing

Feel free to fork this repository and contribute by creating pull requests.

License

This project is licensed under the MIT License.

