#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <stack>
#include <queue>
#include <sstream>

// Status codes matching the NTA palette
enum QuestionStatus {
    NOT_VISITED = 0,
    NOT_ANSWERED = 1,
    ANSWERED = 2,
    MARKED_FOR_REVIEW = 3,
    ANSWERED_AND_MARKED = 4
};

struct Question {
    int id;
    std::string text;
    std::vector<std::string> options;
    int correctOption; // 1-based index
    int marks;
    int negativeMarks;
};

struct StudentResult {
    std::string name;
    int score;
    bool operator<(const StudentResult& other) const {
        return score < other.score; // Max-Heap comparator
    }
};

class ExamDSAEngine {
private:
    std::vector<Question> questions;
    std::unordered_map<int, Question> questionMap;          // O(1) Lookup
    std::unordered_map<int, int> studentResponses;          // qId -> chosen option
    std::unordered_map<int, QuestionStatus> questionState;  // qId -> state
    std::stack<int> navHistory;                             // Stack for back tracking

public:
    ExamDSAEngine() {
        initQuestions();
    }

    void initQuestions() {
        questions.push_back({
            1,
            "What is the capital of India?",
            {"Mumbai", "New Delhi", "Kolkata", "Chennai"},
            2, 4, 1
        });

        questions.push_back({
            2,
            "What is 15 + 27?",
            {"40", "42", "44", "38"},
            2, 4, 1
        });

        questions.push_back({
            3,
            "Which planet is known as the Red Planet?",
            {"Venus", "Jupiter", "Mars", "Saturn"},
            3, 4, 1
        });

        questions.push_back({
            4,
            "What is the chemical formula of water?",
            {"CO2", "H2O", "O2", "NaCl"},
            2, 4, 1
        });

        questions.push_back({
            5,
            "Who wrote the Indian national anthem 'Jana Gana Mana'?",
            {"Bankim Chandra", "Rabindranath Tagore", "Mahatma Gandhi", "Subhash Chandra Bose"},
            2, 4, 1
        });

        questions.push_back({
            6,
            "How many sides does a hexagon have?",
            {"5", "6", "7", "8"},
            2, 4, 1
        });

        questions.push_back({
            7,
            "Which gas do plants absorb from the atmosphere for photosynthesis?",
            {"Oxygen", "Nitrogen", "Carbon Dioxide", "Hydrogen"},
            3, 4, 1
        });

        questions.push_back({
            8,
            "What is the SI unit of force?",
            {"Joule", "Watt", "Newton", "Pascal"},
            3, 4, 1
        });

        questions.push_back({
            9,
            "In which year did India gain independence?",
            {"1942", "1945", "1947", "1950"},
            3, 4, 1
        });

        questions.push_back({
            10,
            "What is the value of the square root of 144?",
            {"10", "11", "12", "14"},
            3, 4, 1
        });

        for (const auto& q : questions) {
            questionMap[q.id] = q;
            questionState[q.id] = NOT_VISITED;
        }
    }

    // Command runner for Java inter-process communication
    void runCommand(const std::string& cmd) {
        std::stringstream ss(cmd);
        std::string action;
        ss >> action;

        if (action == "GET_COUNT") {
            std::cout << questions.size() << "\n";
        }
        else if (action == "GET_Q") {
            int qIndex;
            ss >> qIndex;
            if (qIndex >= 1 && qIndex <= (int)questions.size()) {
                const Question& q = questions[qIndex - 1];
                std::cout << q.id << "|" << q.text << "|";
                for (size_t i = 0; i < q.options.size(); ++i) {
                    std::cout << q.options[i] << (i == q.options.size() - 1 ? "" : "~~~");
                }
                std::cout << "|" << studentResponses[q.id] << "|" << questionState[q.id] << "\n";
            }
        }
        else if (action == "UPDATE") {
            int qId, option, stateCode;
            ss >> qId >> option >> stateCode;
            studentResponses[qId] = option;
            questionState[qId] = static_cast<QuestionStatus>(stateCode);
            navHistory.push(qId);
            std::cout << "OK\n";
        }
        else if (action == "GET_SUMMARY") {
            int notVisited = 0, notAnswered = 0, answered = 0, marked = 0, ansMarked = 0;
            for (const auto& q : questions) {
                switch (questionState[q.id]) {
                    case NOT_VISITED: notVisited++; break;
                    case NOT_ANSWERED: notAnswered++; break;
                    case ANSWERED: answered++; break;
                    case MARKED_FOR_REVIEW: marked++; break;
                    case ANSWERED_AND_MARKED: ansMarked++; break;
                }
            }
            std::cout << notVisited << " " << notAnswered << " " << answered << " " 
                      << marked << " " << ansMarked << "\n";
        }
        else if (action == "SUBMIT") {
            int totalScore = 0;
            for (const auto& q : questions) {
                int userAns = studentResponses[q.id];
                QuestionStatus st = questionState[q.id];

                // Answered or Answered & Marked for Review are evaluated
                if (st == ANSWERED || st == ANSWERED_AND_MARKED) {
                    if (userAns == q.correctOption) totalScore += q.marks;
                    else totalScore -= q.negativeMarks;
                }
            }

            // Max-Heap Priority Queue for Rank list demonstration
            std::priority_queue<StudentResult> pq;
            pq.push({"[Your Name]", totalScore});
            pq.push({"Candidate A", 12});
            pq.push({"Candidate B", 8});

            std::cout << "SCORE|" << totalScore << "\n";
        }
    }
};

int main(int argc, char* argv[]) {
    ExamDSAEngine engine;
    if (argc > 1) {
        // Run single command passed via arguments
        std::string command;
        for (int i = 1; i < argc; ++i) {
            command += argv[i];
            if (i < argc - 1) command += " ";
        }
        engine.runCommand(command);
    } else {
        // Interactive loop for standard I/O
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == "EXIT") break;
            engine.runCommand(line);
        }
    }
    return 0;
}
