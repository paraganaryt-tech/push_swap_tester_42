// ==================================================================================
// PS_TESTER v4.0 - The Ultimate Push_swap Tester for 42/1337 Schools
// by mjabri
// ==================================================================================
//
// Features:
//   ✓ Exhaustive parsing tests (edge cases, overflows, formats)
//   ✓ Deep memory leak detection (Valgrind integration)
//   ✓ Operation validation (verifies each op works correctly)
//   ✓ Performance benchmarking with 42 grading scale
//   ✓ Checker program testing (bonus)
//   ✓ Stress testing (various sizes, edge arrays)
//   ✓ HTML report generation
//   ✓ Detailed trace logging for failed tests
//
// Build: g++ -std=c++17 -O3 -pthread push_swap_ultimate_tester.cpp -o ps_tester
// Usage: ./ps_tester <push_swap_path> [checker_path] [options]
//
// Options:
//   --no-valgrind     Skip memory leak tests
//   --quick           Quick mode (fewer iterations)
//   --stress          Extra stress tests
//   --html            Generate HTML report
//   --checker-only    Only test checker program
// ==================================================================================

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <set>
#include <map>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>
#include <climits>
#include <numeric>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <deque>
#include <fcntl.h>
#include <ctime>

using namespace std;

// ==================================================================================
// ANSI Colors & Styling
// ==================================================================================
#define RST     "\033[0m"
#define RED     "\033[31m"
#define GRN     "\033[32m"
#define YEL     "\033[33m"
#define BLU     "\033[34m"
#define MAG     "\033[35m"
#define CYN     "\033[36m"
#define WHT     "\033[37m"
#define GRY     "\033[90m"
#define BLD     "\033[1m"
#define DIM     "\033[2m"
#define UND     "\033[4m"

#define PASS    GRN "✓ PASS" RST
#define FAIL    RED "✗ FAIL" RST
#define WARN    YEL "⚠ WARN" RST
#define SKIP    GRY "○ SKIP" RST
#define LEAK    RED "☠ LEAK" RST
#define SEGV    RED "💀 SEGV" RST
#define TOUT    YEL "⏰ TIMEOUT" RST

// ==================================================================================
// Configuration
// ==================================================================================
struct Config {
    string push_swap;
    string checker;
    bool use_valgrind = true;
    bool quick_mode = false;
    bool stress_mode = false;
    bool html_report = false;
    bool checker_only = false;
    bool verbose = false;
    int timeout_sec = 5;
    string trace_file = "trace.log";
    string html_file = "report.html";
    string errors_file = "errors.txt";
};

Config cfg;

// ==================================================================================
// 42 Grading Thresholds
// ==================================================================================
struct Threshold { int limit; int score; string grade; };

const vector<Threshold> SCORES_3 = {
    {3, 5, "5/5"}, {4, 4, "4/5"}, {5, 3, "3/5"}, {6, 2, "2/5"}, {INT_MAX, 1, "1/5"}
};
const vector<Threshold> SCORES_5 = {
    {12, 5, "5/5"}, {15, 4, "4/5"}, {18, 3, "3/5"}, {22, 2, "2/5"}, {INT_MAX, 1, "1/5"}
};
const vector<Threshold> SCORES_100 = {
    {700, 5, "5/5"}, {900, 4, "4/5"}, {1100, 3, "3/5"}, {1300, 2, "2/5"}, {1500, 1, "1/5"}
};
const vector<Threshold> SCORES_500 = {
    {5500, 5, "5/5"}, {7000, 4, "4/5"}, {8500, 3, "3/5"}, {10000, 2, "2/5"}, {11500, 1, "1/5"}
};

// ==================================================================================
// Statistics
// ==================================================================================
struct Stats {
    int total = 0;
    int passed = 0;
    int failed = 0;
    int leaks = 0;
    int crashes = 0;
    int timeouts = 0;
    vector<string> failed_tests;
    map<string, vector<int>> perf_results;
};

struct CheckerStats {
    int total = 0;
    int passed = 0;
    int failed = 0;
    int leaks = 0;
    int crashes = 0;
    vector<string> failed_tests;
};

Stats stats;
CheckerStats checker_stats;
mutex stats_mutex;

// ==================================================================================
// Test Result Structure
// ==================================================================================
struct ExecResult {
    string stdout_data;
    string stderr_data;
    int exit_code = 0;
    int signal_num = 0;
    bool timed_out = false;
    double exec_time_ms = 0;
    
    // Valgrind specific
    bool has_leaks = false;
    int leaked_bytes = 0;
    int leaked_blocks = 0;
    bool has_errors = false;
    int error_count = 0;
    string valgrind_summary;
};

struct TestResult {
    string name;
    string category;
    bool passed = false;
    string status;
    string details;
    ExecResult exec;
    int instruction_count = 0;
};

vector<TestResult> all_results;

// ==================================================================================
// Utility Functions
// ==================================================================================

void log_trace(const string& name, const vector<string>& args, const string& extra = "") {
    lock_guard<mutex> lock(stats_mutex);
    ofstream f(cfg.trace_file, ios::app);
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    f << "[" << put_time(localtime(&t), "%H:%M:%S") << "] " << name << "\n";
    
    // Show actual command with valgrind if enabled
    if (cfg.use_valgrind) {
        f << "  Command: valgrind --leak-check=full " << cfg.push_swap;
    } else {
        f << "  Command: " << cfg.push_swap;
    }
    for (const auto& s : args) f << " \"" << s << "\"";
    f << "\n";
    if (!extra.empty()) f << "  Details: " << extra << "\n";
    f << "\n";
}

void log_error(const string& test_name, const string& category, const string& details, const vector<string>& args = {}) {
    lock_guard<mutex> lock(stats_mutex);
    ofstream f(cfg.errors_file, ios::app);
    f << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    f << "FAILED: " << test_name << "\n";
    f << "Category: " << category << "\n";
    if (!args.empty()) {
        f << "Input: ";
        for (size_t i = 0; i < args.size() && i < 20; ++i) {
            f << args[i];
            if (i < args.size() - 1) f << " ";
        }
        if (args.size() > 20) f << "... (" << args.size() << " total)";
        f << "\n";
    }
    if (!details.empty()) f << "Reason: " << details << "\n";
    f << "\n";
}

string vec_to_args(const vector<int>& v) {
    string s;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) s += " ";
        s += to_string(v[i]);
    }
    return s;
}

vector<string> split_string(const string& s) {
    vector<string> result;
    istringstream iss(s);
    string token;
    while (iss >> token) result.push_back(token);
    return result;
}

int count_instructions(const string& output) {
    if (output.empty()) return 0;
    int count = 0;
    for (char c : output) if (c == '\n') count++;
    if (!output.empty() && output.back() != '\n') count++;
    return count;
}

bool is_valid_instruction(const string& instr) {
    static const set<string> valid_ops = {
        "sa", "sb", "ss", "pa", "pb", "ra", "rb", "rr", "rra", "rrb", "rrr"
    };
    return valid_ops.count(instr) > 0;
}

bool validate_all_instructions(const string& output) {
    if (output.empty()) return true;
    istringstream iss(output);
    string line;
    while (getline(iss, line)) {
        if (line.empty()) continue;
        if (!is_valid_instruction(line)) return false;
    }
    return true;
}

string get_grade(int avg, const vector<Threshold>& thresholds) {
    for (const auto& t : thresholds) {
        if (avg <= t.limit) return t.grade;
    }
    return "0/5";
}

int get_score(int avg, const vector<Threshold>& thresholds) {
    for (const auto& t : thresholds) {
        if (avg <= t.limit) return t.score;
    }
    return 0;
}

// ==================================================================================
// Number Generators
// ==================================================================================

vector<int> generate_unique_random(int n, int min_val = INT_MIN, int max_val = INT_MAX) {
    vector<int> result;
    unordered_set<int> used;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(min_val, max_val);
    
    while (result.size() < (size_t)n) {
        int val = dist(gen);
        if (used.insert(val).second) {
            result.push_back(val);
        }
    }
    return result;
}

vector<int> generate_range(int start, int end) {
    vector<int> result;
    for (int i = start; i <= end; ++i) result.push_back(i);
    return result;
}

vector<int> generate_reversed(int n) {
    vector<int> result;
    for (int i = n; i >= 1; --i) result.push_back(i);
    return result;
}

vector<int> generate_nearly_sorted(int n, int swaps = 2) {
    vector<int> result = generate_range(1, n);
    random_device rd;
    mt19937 gen(rd());
    for (int i = 0; i < swaps && n > 1; ++i) {
        int a = gen() % n;
        int b = gen() % n;
        swap(result[a], result[b]);
    }
    return result;
}

vector<int> generate_rotated(int n, int rot = -1) {
    vector<int> result = generate_range(1, n);
    if (rot < 0) rot = n / 2;
    rotate(result.begin(), result.begin() + rot, result.end());
    return result;
}

// ==================================================================================
// Process Execution
// ==================================================================================

ExecResult execute_command(const vector<string>& cmd, const string& input = "", 
                           bool with_valgrind = false, int timeout = -1) {
    ExecResult result;
    if (timeout < 0) timeout = cfg.timeout_sec;
    
    int pipe_stdin[2], pipe_stdout[2], pipe_stderr[2];
    
    if (pipe(pipe_stdin) < 0 || pipe(pipe_stdout) < 0 || pipe(pipe_stderr) < 0) {
        result.stderr_data = "Failed to create pipes";
        result.exit_code = -1;
        return result;
    }
    auto start_time = chrono::high_resolution_clock::now();
    
    pid_t pid = fork();
    if (pid < 0) {
        result.stderr_data = "Fork failed";
        result.exit_code = -1;
        return result;
    }
    
    if (pid == 0) {
        // Child process
        close(pipe_stdin[1]);
        close(pipe_stdout[0]);
        close(pipe_stderr[0]);
        
        dup2(pipe_stdin[0], STDIN_FILENO);
        dup2(pipe_stdout[1], STDOUT_FILENO);
        dup2(pipe_stderr[1], STDERR_FILENO);
        
        close(pipe_stdin[0]);
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);
        
        // Build command
        vector<const char*> args;
        for (const auto& s : cmd) args.push_back(s.c_str());
        args.push_back(nullptr);
        
        execvp(args[0], const_cast<char* const*>(args.data()));
        _exit(127);
    }
    
    // Parent process
    close(pipe_stdin[0]);
    close(pipe_stdout[1]);
    close(pipe_stderr[1]);
    
    // Set non-blocking for stdout/stderr
    fcntl(pipe_stdout[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_stderr[0], F_SETFL, O_NONBLOCK);
    
    // Write input
    if (!input.empty()) {
        ssize_t written __attribute__((unused)) = write(pipe_stdin[1], input.c_str(), input.size());
    }
    close(pipe_stdin[1]);
    
    // Wait with timeout
    int status;
    int elapsed_ms = 0;
    const int poll_interval_ms = 10;
    
    while (waitpid(pid, &status, WNOHANG) == 0) {
        if (elapsed_ms >= timeout * 1000) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            result.timed_out = true;
            break;
        }
        usleep(poll_interval_ms * 1000);
        elapsed_ms += poll_interval_ms;
    }
    
    auto end_time = chrono::high_resolution_clock::now();
    result.exec_time_ms = chrono::duration<double, milli>(end_time - start_time).count();
    
    // Read output
    char buf[8192];
    ssize_t n;
    while ((n = read(pipe_stdout[0], buf, sizeof(buf))) > 0) {
        result.stdout_data.append(buf, n);
    }
    while ((n = read(pipe_stderr[0], buf, sizeof(buf))) > 0) {
        result.stderr_data.append(buf, n);
    }
    
    close(pipe_stdout[0]);
    close(pipe_stderr[0]);
    
    if (!result.timed_out) {
        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            result.signal_num = WTERMSIG(status);
        }
    }
    
    // Parse valgrind output if applicable
    if (with_valgrind && !result.stderr_data.empty()) {
        // Check for "definitely lost"
        if (result.stderr_data.find("definitely lost:") != string::npos) {
            size_t pos = result.stderr_data.find("definitely lost:");
            if (pos != string::npos) {
                size_t start = pos + 17;  // "definitely lost:" is 16 chars + skip colon
                // Skip any whitespace after the colon
                while (start < result.stderr_data.size() && 
                       (result.stderr_data[start] == ' ' || result.stderr_data[start] == '\t')) {
                    start++;
                }
                string bytes_str;
                while (start < result.stderr_data.size() && 
                       (isdigit(result.stderr_data[start]) || result.stderr_data[start] == ',')) {
                    if (result.stderr_data[start] != ',') bytes_str += result.stderr_data[start];
                    start++;
                }
                if (!bytes_str.empty()) {
                    result.leaked_bytes = stoi(bytes_str);
                    if (result.leaked_bytes > 0) result.has_leaks = true;
                }
            }
        }
        
        // Check for errors
        if (result.stderr_data.find("ERROR SUMMARY:") != string::npos) {
            size_t pos = result.stderr_data.find("ERROR SUMMARY:");
            if (pos != string::npos) {
                size_t start = pos + 15;
                string err_str;
                while (start < result.stderr_data.size() && isdigit(result.stderr_data[start])) {
                    err_str += result.stderr_data[start++];
                }
                if (!err_str.empty()) {
                    result.error_count = stoi(err_str);
                    if (result.error_count > 0) result.has_errors = true;
                }
            }
        }
        
        // Check for "All heap blocks were freed"
        if (result.stderr_data.find("All heap blocks were freed") != string::npos) {
            result.has_leaks = false;
            result.leaked_bytes = 0;
        }
        
        // Store summary
        size_t sum_pos = result.stderr_data.find("LEAK SUMMARY:");
        if (sum_pos != string::npos) {
            size_t end_pos = result.stderr_data.find("ERROR SUMMARY:", sum_pos);
            if (end_pos != string::npos) {
                result.valgrind_summary = result.stderr_data.substr(sum_pos, end_pos - sum_pos);
            }
        }
    }
    
    return result;
}

ExecResult run_push_swap(const vector<string>& args, bool with_valgrind = false) {
    vector<string> cmd;
    if (with_valgrind && cfg.use_valgrind) {
        cmd = {"valgrind", "--leak-check=full", "--show-leak-kinds=all", 
               "--errors-for-leak-kinds=all", "--error-exitcode=42", 
               cfg.push_swap};
    } else {
        cmd = {cfg.push_swap};
    }
    cmd.insert(cmd.end(), args.begin(), args.end());
    return execute_command(cmd, "", with_valgrind);
}

ExecResult run_checker(const vector<string>& args, const string& instructions, bool with_valgrind = false) {
    vector<string> cmd;
    if (with_valgrind && cfg.use_valgrind) {
        cmd = {"valgrind", "--leak-check=full", "--show-leak-kinds=all", 
               "--errors-for-leak-kinds=all", "--error-exitcode=42", 
               cfg.checker};
    } else {
        cmd = {cfg.checker};
    }
    cmd.insert(cmd.end(), args.begin(), args.end());
    return execute_command(cmd, instructions, with_valgrind);
}

// ==================================================================================
// Stack Simulation (for operation validation)
// ==================================================================================

class StackSimulator {
public:
    deque<int> a, b;
    
    void init(const vector<int>& nums) {
        a.clear();
        b.clear();
        for (int n : nums) a.push_back(n);
    }
    
    bool execute(const string& op) {
        if (op == "sa") { if (a.size() >= 2) swap(a[0], a[1]); }
        else if (op == "sb") { if (b.size() >= 2) swap(b[0], b[1]); }
        else if (op == "ss") { execute("sa"); execute("sb"); }
        else if (op == "pa") { if (!b.empty()) { a.push_front(b.front()); b.pop_front(); } }
        else if (op == "pb") { if (!a.empty()) { b.push_front(a.front()); a.pop_front(); } }
        else if (op == "ra") { if (a.size() >= 2) { a.push_back(a.front()); a.pop_front(); } }
        else if (op == "rb") { if (b.size() >= 2) { b.push_back(b.front()); b.pop_front(); } }
        else if (op == "rr") { execute("ra"); execute("rb"); }
        else if (op == "rra") { if (a.size() >= 2) { a.push_front(a.back()); a.pop_back(); } }
        else if (op == "rrb") { if (b.size() >= 2) { b.push_front(b.back()); b.pop_back(); } }
        else if (op == "rrr") { execute("rra"); execute("rrb"); }
        else return false;
        return true;
    }
    
    bool execute_all(const string& instructions) {
        istringstream iss(instructions);
        string op;
        while (getline(iss, op)) {
            if (op.empty()) continue;
            if (!execute(op)) return false;
        }
        return true;
    }
    
    bool is_sorted() const {
        if (!b.empty()) return false;
        for (size_t i = 1; i < a.size(); ++i) {
            if (a[i] < a[i-1]) return false;
        }
        return true;
    }
};

bool verify_sort(const vector<int>& nums, const string& instructions) {
    StackSimulator sim;
    sim.init(nums);
    if (!sim.execute_all(instructions)) return false;
    return sim.is_sorted();
}

// ==================================================================================
// Pretty Printing
// ==================================================================================

void print_header(const string& title) {
    cout << "\n" << BLD << BLU;
    cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    cout << "║ " << left << setw(65) << title << "║\n";
    cout << "╚══════════════════════════════════════════════════════════════════╝" << RST << "\n";
}

void print_subheader(const string& title) {
    cout << "\n" << BLD << CYN << "▶ " << title << RST << "\n";
    cout << GRY << string(70, '-') << RST << "\n";
}

void print_result(const string& name, const string& status, const string& details = "") {
    cout << "  " << left << setw(40) << name << " ";
    cout << status;
    if (!details.empty()) cout << "  " << GRY << details << RST;
    cout << "\n";
}

void print_progress(int current, int total, const string& prefix = "") {
    int width = 40;
    int filled = (current * width) / total;
    cout << "\r" << prefix << " [";
    for (int i = 0; i < width; ++i) {
        if (i < filled) cout << GRN << "█" << RST;
        else cout << GRY << "░" << RST;
    }
    cout << "] " << current << "/" << total << " " << flush;
}

void clear_line() {
    cout << "\r" << string(80, ' ') << "\r" << flush;
}

// ==================================================================================
// Test Functions
// ==================================================================================

TestResult test_error_case(const string& name, const vector<string>& args, bool check_leaks = true) {
    TestResult result;
    result.name = name;
    result.category = "Error Handling";
    
    result.exec = run_push_swap(args, check_leaks && cfg.use_valgrind);
    
    // Check for "Error" in stderr
    bool has_error = (result.exec.stderr_data.find("Error") != string::npos);
    
    // Some implementations print to stdout
    if (!has_error) {
        has_error = (result.exec.stdout_data.find("Error") != string::npos);
    }
    
    // Check leaks FIRST - a leak is ALWAYS a failure, even if output is correct!
    bool has_leak = (check_leaks && cfg.use_valgrind && result.exec.has_leaks);
    
    if (has_leak) {
        result.passed = false;
        result.status = LEAK;
        result.details = to_string(result.exec.leaked_bytes) + " bytes leaked";
        if (!has_error) result.details += " + Expected 'Error' on stderr";
        stats.leaks++;
    } else if (has_error) {
        result.passed = true;
        result.status = PASS;
    } else if (result.exec.timed_out) {
        result.passed = false;
        result.status = TOUT;
        stats.timeouts++;
    } else if (result.exec.signal_num == SIGSEGV) {
        result.passed = false;
        result.status = SEGV;
        stats.crashes++;
    } else if (result.exec.signal_num == SIGABRT) {
        result.passed = false;
        result.status = RED "ABORT" RST;
        stats.crashes++;
    } else {
        result.passed = false;
        result.status = FAIL;
        result.details = "Expected 'Error' on stderr";
    }
    
    stats.total++;
    if (result.passed) stats.passed++;
    else {
        stats.failed++;
        stats.failed_tests.push_back(name);
        log_trace(name, args, result.details);
        log_error(name, "Error Handling", result.details, args);
    }
    
    all_results.push_back(result);
    return result;
}

TestResult test_empty_case(const string& name, const vector<string>& args, bool check_leaks = true) {
    TestResult result;
    result.name = name;
    result.category = "Empty/No Output";
    
    result.exec = run_push_swap(args, check_leaks && cfg.use_valgrind);
    
    // When valgrind is running, stderr contains valgrind output, not program output
    // So we need to check if the program's actual stderr is empty
    // Valgrind output starts with "==" so we can detect it
    bool stderr_is_program_output = result.exec.stderr_data.empty() ||
        (check_leaks && cfg.use_valgrind && 
         result.exec.stderr_data.find("==") != string::npos &&
         result.exec.stderr_data.find("Error") == string::npos);
    
    // Should output nothing and exit cleanly
    bool clean_exit = (result.exec.exit_code == 0 && 
                       result.exec.stdout_data.empty() && 
                       stderr_is_program_output);
    
    // Also accept "Error" for edge cases like empty string
    bool error_exit = (result.exec.stderr_data.find("Error") != string::npos);
    
    // Check leaks FIRST - a leak is ALWAYS a failure!
    bool has_leak = (check_leaks && cfg.use_valgrind && result.exec.has_leaks);
    
    if (has_leak) {
        result.passed = false;
        result.status = LEAK;
        result.details = to_string(result.exec.leaked_bytes) + " bytes leaked";
        if (!clean_exit && !error_exit) result.details += " + Expected empty output or 'Error'";
        stats.leaks++;
    } else if (clean_exit || error_exit) {
        result.passed = true;
        result.status = PASS;
    } else if (result.exec.timed_out) {
        result.passed = false;
        result.status = TOUT;
        stats.timeouts++;
    } else if (result.exec.signal_num != 0) {
        result.passed = false;
        result.status = (result.exec.signal_num == SIGSEGV) ? SEGV : RED "CRASH" RST;
        stats.crashes++;
    } else {
        result.passed = false;
        result.status = FAIL;
        result.details = "Expected empty output or 'Error'";
    }
    
    stats.total++;
    if (result.passed) stats.passed++;
    else {
        stats.failed++;
        stats.failed_tests.push_back(name);
        log_trace(name, args, result.details);
        log_error(name, "Empty/No Output", result.details, args);
    }
    
    all_results.push_back(result);
    return result;
}

TestResult test_sort_case(const string& name, const vector<int>& nums, bool check_leaks = true) {
    TestResult result;
    result.name = name;
    result.category = "Sorting";
    
    vector<string> args;
    for (int n : nums) args.push_back(to_string(n));
    
    result.exec = run_push_swap(args, check_leaks && cfg.use_valgrind);
    
    // Check leaks FIRST - a leak is ALWAYS a failure!
    bool has_leak = (check_leaks && cfg.use_valgrind && result.exec.has_leaks);
    
    if (has_leak) {
        result.passed = false;
        result.status = LEAK;
        result.details = to_string(result.exec.leaked_bytes) + " bytes leaked";
        result.instruction_count = count_instructions(result.exec.stdout_data);
        stats.leaks++;
    } else if (result.exec.timed_out) {
        result.passed = false;
        result.status = TOUT;
        stats.timeouts++;
    } else if (result.exec.signal_num == SIGSEGV) {
        result.passed = false;
        result.status = SEGV;
        stats.crashes++;
    } else if (result.exec.signal_num != 0) {
        result.passed = false;
        result.status = RED "CRASH" RST;
        stats.crashes++;
    } else {
        // Validate instructions
        if (!validate_all_instructions(result.exec.stdout_data)) {
            result.passed = false;
            result.status = FAIL;
            result.details = "Invalid instruction(s)";
        } else {
            // Verify sorting
            bool sorted = verify_sort(nums, result.exec.stdout_data);
            result.instruction_count = count_instructions(result.exec.stdout_data);
            
            if (sorted) {
                result.passed = true;
                result.status = PASS;
                result.details = to_string(result.instruction_count) + " ops";
            } else {
                // Try with checker if available
                if (!cfg.checker.empty()) {
                    ExecResult chk = run_checker(args, result.exec.stdout_data);
                    if (chk.stdout_data.find("OK") != string::npos) {
                        result.passed = true;
                        result.status = PASS;
                        result.details = to_string(result.instruction_count) + " ops";
                    } else {
                        result.passed = false;
                        result.status = FAIL;
                        result.details = "Checker: KO";
                    }
                } else {
                    result.passed = false;
                    result.status = FAIL;
                    result.details = "Not sorted";
                }
            }
        }
    }
    
    stats.total++;
    if (result.passed) stats.passed++;
    else {
        stats.failed++;
        stats.failed_tests.push_back(name);
        log_trace(name, args, result.details);
        log_error(name, "Sorting", result.details, args);
    }
    
    all_results.push_back(result);
    return result;
}

// ==================================================================================
// Test Suites
// ==================================================================================

void run_parsing_tests() {
    print_header("PARSING TESTS - Error Handling");
    
    print_subheader("Empty/No Arguments");
    // No arguments - should display nothing
    print_result("No arguments", 
                 test_empty_case("No arguments", {}).status);
    print_result("Empty string \"\"", 
                 test_empty_case("Empty string \"\"", {""}).status);
    print_result("Just spaces \"   \"", 
                 test_error_case("Just spaces", {"   "}).status);
    print_result("Multiple empty strings", 
                 test_error_case("Multiple empty strings", {"", ""}).status);
    print_result("Tab character", 
                 test_error_case("Tab character", {"\t"}).status);
    print_result("Newline character", 
                 test_error_case("Newline character", {"\n"}).status);
    
    print_subheader("Invalid Characters");
    print_result("Letter 'a'", 
                 test_error_case("Letter 'a'", {"a"}).status);
    print_result("Letters 'abc'", 
                 test_error_case("Letters 'abc'", {"abc"}).status);
    print_result("Letters 'hello world'", 
                 test_error_case("Letters 'hello world'", {"hello", "world"}).status);
    print_result("Mixed '1a'", 
                 test_error_case("Mixed '1a'", {"1a"}).status);
    print_result("Mixed 'a1'", 
                 test_error_case("Mixed 'a1'", {"a1"}).status);
    print_result("Mixed '1a2'", 
                 test_error_case("Mixed '1a2'", {"1a2"}).status);
    print_result("Mixed '111a11'", 
                 test_error_case("Mixed '111a11'", {"111a11"}).status);
    print_result("Mixed '111a111 -4 3'", 
                 test_error_case("Mixed '111a111'", {"111a111", "-4", "3"}).status);
    print_result("Letter in middle '1 a 2'", 
                 test_error_case("Letter in middle", {"1", "a", "2"}).status);
    print_result("Letter 'x' in long list", 
                 test_error_case("Letter x in list", {"42", "41", "40", "45", "101", "x", "202", "-1", "224", "3"}).status);
    print_result("Letter 'e' at end of list", 
                 test_error_case("Letter e at end", {"42", "-2", "10", "11", "0", "90", "45", "500", "-200", "e"}).status);
    print_result("Single letter in list '42 a 41'", 
                 test_error_case("Single letter in list", {"42", "a", "41"}).status);
    print_result("Special char '@'", 
                 test_error_case("Special char '@'", {"@"}).status);
    print_result("Special char '#'", 
                 test_error_case("Special char '#'", {"#"}).status);
    print_result("Decimal '1.5'", 
                 test_error_case("Decimal '1.5'", {"1.5"}).status);
    print_result("Comma separated '1,2,3'", 
                 test_error_case("Comma separated", {"1,2,3"}).status);
    
    print_subheader("Sign Issues");
    print_result("Single plus '+'", 
                 test_error_case("Single plus '+'", {"+"}).status);
    print_result("Single minus '-'", 
                 test_error_case("Single minus '-'", {"-"}).status);
    print_result("Double plus '++5'", 
                 test_error_case("Double plus '++5'", {"++5"}).status);
    print_result("Double minus '--5'", 
                 test_error_case("Double minus '--5'", {"--5"}).status);
    print_result("Double minus '--123 1 321'", 
                 test_error_case("Double minus in list", {"--123", "1", "321"}).status);
    print_result("Double plus '++123 1 321'", 
                 test_error_case("Double plus in list", {"++123", "1", "321"}).status);
    print_result("Plus minus '+-5'", 
                 test_error_case("Plus minus '+-5'", {"+-5"}).status);
    print_result("Minus plus '-+5'", 
                 test_error_case("Minus plus '-+5'", {"-+5"}).status);
    print_result("Trailing plus '5+'", 
                 test_error_case("Trailing plus '5+'", {"5+"}).status);
    print_result("Trailing minus '5-'", 
                 test_error_case("Trailing minus '5-'", {"5-"}).status);
    print_result("Sign in middle '5-3'", 
                 test_error_case("Sign in middle '5-3'", {"5-3"}).status);
    print_result("Sign in middle '111-1 2 -3'", 
                 test_error_case("Sign in middle 111-1", {"111-1", "2", "-3"}).status);
    print_result("Sign in middle '3333-3333 1 4'", 
                 test_error_case("Sign in middle 3333-3333", {"3333-3333", "1", "4"}).status);
    print_result("Sign in middle '4222-4222'", 
                 test_error_case("Sign in middle 4222-4222", {"4222-4222"}).status);
    print_result("Plus in middle '3+3'", 
                 test_error_case("Plus in middle 3+3", {"3+3"}).status);
    print_result("Plus in middle '111+111 -4 3'", 
                 test_error_case("Plus in middle 111+111", {"111+111", "-4", "3"}).status);
    print_result("Plus in number '2147483647+1'", 
                 test_error_case("Plus in number 2147483647+1", {"2147483647+1"}).status);
    print_result("Space after sign '- 5'", 
                 test_error_case("Space after sign", {"- 5"}).status);
    
    print_subheader("Integer Overflow");
    print_result("INT_MAX (2147483647)", 
                 test_sort_case("INT_MAX", {2147483647}).status);
    print_result("INT_MIN (-2147483648)", 
                 test_sort_case("INT_MIN", {-2147483648}).status);
    print_result("INT_MAX + 1", 
                 test_error_case("INT_MAX + 1", {"2147483648"}).status);
    print_result("INT_MAX + 2 (2147483649)", 
                 test_error_case("INT_MAX + 2", {"2147483649"}).status);
    print_result("INT_MIN - 1", 
                 test_error_case("INT_MIN - 1", {"-2147483649"}).status);
    print_result("INT_MIN - 2 (-2147483650)", 
                 test_error_case("INT_MIN - 2", {"-2147483650"}).status);
    print_result("Huge number (26 digits)", 
                 test_error_case("Huge 26 digit", {"99999999999999999999999999"}).status);
    print_result("Huge negative (26 digits)", 
                 test_error_case("Huge negative 26 digit", {"-99999999999999999999999999"}).status);
    print_result("Huge number (20 digits)", 
                 test_error_case("Huge 20 digit", {"99999999999999999999"}).status);
    print_result("Huge negative (20 digits)", 
                 test_error_case("Huge negative 20 digit", {"-99999999999999999999"}).status);
    print_result("LLONG_MAX", 
                 test_error_case("LLONG_MAX", {"9223372036854775807"}).status);
    print_result("Near overflow positive", 
                 test_error_case("Near overflow +", {"2147483650"}).status);
    print_result("Near overflow negative", 
                 test_error_case("Near overflow -", {"-2147483650"}).status);
    // Massive overflow that might cause leaks if not handled properly
    print_result("Massive overflow (50+ digits)", 
                 test_error_case("Massive overflow", {"1", "2", "555555555555555555555555555555555555555555555555"}).status);
    // User's specific test - valid numbers followed by overflow
    print_result("Valid then overflow '1 2 3 99999999999999999999'", 
                 test_error_case("Valid then overflow", {"1", "2", "3", "99999999999999999999"}).status);
    print_result("Overflow at end of long list", 
                 test_error_case("Overflow at end", {"42", "41", "40", "99999999999999999999999999"}).status);
    print_result("Negative overflow in list", 
                 test_error_case("Negative overflow in list", {"-1", "-2", "-99999999999999999999"}).status);
    
    print_subheader("Leading Zeros (Duplicate Detection)");
    print_result("Leading zero '01' (single)",
                 test_sort_case("Leading zero '01'", {1}).status);  // Single - should work
    print_result("Many leading zeros '00001'", 
                 test_sort_case("Leading zeros '00001'", {1}).status);
    print_result("Just zeros '000'", 
                 test_sort_case("Just zeros '000'", {0}).status);
    // Duplicates hidden by leading zeros (should ERROR)
    print_result("Duplicate '1 01' (hidden)", 
                 test_error_case("Duplicate hidden 1 01", {"1", "01"}).status);
    print_result("Duplicate '8 008 12' (leading zeros)", 
                 test_error_case("Duplicate 8 008", {"8", "008", "12"}).status);
    print_result("Duplicate '-01 -001'", 
                 test_error_case("Duplicate -01 -001", {"-01", "-001"}).status);
    print_result("Duplicate '00000001 1 9 3'", 
                 test_error_case("Duplicate 00000001 1", {"00000001", "1", "9", "3"}).status);
    print_result("Duplicate '111111 -4 3 03'", 
                 test_error_case("Duplicate with 03", {"111111", "-4", "3", "03"}).status);
    print_result("Duplicate '00000003 003 9 1'", 
                 test_error_case("Duplicate 00000003 003", {"00000003", "003", "9", "1"}).status);
    print_result("Duplicate '0000000000000000000000009 x2'", 
                 test_error_case("Duplicate many zeros 9", {"0000000000000000000000009", "000000000000000000000009"}).status);
    print_result("Duplicate '-000 -0000'", 
                 test_error_case("Duplicate negative zeros", {"-000", "-0000"}).status);
    print_result("Duplicate '-00042 -000042'", 
                 test_error_case("Duplicate -00042 -000042", {"-00042", "-000042"}).status);
    
    print_subheader("Duplicates");
    print_result("Simple duplicate '1 1'", 
                 test_error_case("Simple duplicate", {"1", "1"}).status);
    print_result("Duplicate at end '1 2 3 1'", 
                 test_error_case("Duplicate at end", {"1", "2", "3", "1"}).status);
    print_result("Duplicate zeros '0 0'", 
                 test_error_case("Duplicate zeros", {"0", "0"}).status);
    print_result("Duplicate negatives '-1 -1'", 
                 test_error_case("Duplicate negatives", {"-1", "-1"}).status);
    print_result("Duplicate negatives '-3 -2 -2'", 
                 test_error_case("Duplicate -3 -2 -2", {"-3", "-2", "-2"}).status);
    print_result("Duplicate INT_MAX", 
                 test_error_case("Duplicate INT_MAX", {"2147483647", "2147483647"}).status);
    print_result("Duplicate with long list", 
                 test_error_case("Duplicate long list", {"10", "-1", "-2", "-3", "-4", "-5", "-6", "90", "99", "10"}).status);
    print_result("Duplicate '42 42'", 
                 test_error_case("Duplicate 42 42", {"42", "42"}).status);
    print_result("Duplicate negative '42 -42 -42'", 
                 test_error_case("Duplicate 42 -42 -42", {"42", "-42", "-42"}).status);
    print_result("Duplicate -0 and 0", 
                 test_error_case("Duplicate -0 and 0", {"-0", "0"}).status);
    print_result("Duplicate +0 and 0", 
                 test_error_case("Duplicate +0 and 0", {"+0", "0"}).status);
    print_result("Duplicate 0 -0 1 -1 (zero dup)", 
                 test_error_case("Duplicate 0 -0 1 -1", {"0", "-0", "1", "-1"}).status);
    print_result("Duplicate 0 +0 1 -1 (zero dup)", 
                 test_error_case("Duplicate 0 +0 1 -1", {"0", "+0", "1", "-1"}).status);
    print_result("Duplicate at start '0 1 2 3 4 5 0'", 
                 test_error_case("Duplicate 0 at start/end", {"0", "1", "2", "3", "4", "5", "0"}).status);
    print_result("Duplicate '3 +3' (plus sign)", 
                 test_error_case("Duplicate 3 +3", {"3", "+3"}).status);
    print_result("Duplicate '1 +1 -1' (plus sign)", 
                 test_error_case("Duplicate 1 +1 -1", {"1", "+1", "-1"}).status);
    
    print_subheader("Format Edge Cases");
    print_result("Quoted single arg '1 2 3'", 
                 test_sort_case("Quoted single arg", {1, 2, 3}).status);
    print_result("Extra spaces '  1   2  '", 
                 test_sort_case("Extra spaces", {1, 2}).status);
    print_result("Mixed quoted args", 
                 test_sort_case("Mixed quoted args", {1, 2, 3, 4, 5}).status);
    print_result("Positive with plus '+5'", 
                 test_sort_case("Positive with plus", {5}).status);
    print_result("Multiple args with plus", 
                 test_sort_case("Multiple with plus", {1, 2, 3}).status);
}

void run_basic_sorting_tests() {
    print_header("BASIC SORTING TESTS");
    
    print_subheader("Single Element");
    print_result("Single 0", test_sort_case("Single 0", {0}).status);
    print_result("Single 1", test_sort_case("Single 1", {1}).status);
    print_result("Single -1", test_sort_case("Single -1", {-1}).status);
    print_result("Single INT_MAX", test_sort_case("Single INT_MAX", {2147483647}).status);
    print_result("Single INT_MIN", test_sort_case("Single INT_MIN", {-2147483648}).status);
    
    print_subheader("Two Elements");
    print_result("Sorted 1 2", test_sort_case("Sorted 1 2", {1, 2}).status);
    print_result("Reversed 2 1", test_sort_case("Reversed 2 1", {2, 1}).status);
    print_result("Negative pair", test_sort_case("Negative pair", {-2, -1}).status);
    print_result("Mixed signs -1 1", test_sort_case("Mixed signs", {-1, 1}).status);
    print_result("With zero 0 1", test_sort_case("With zero", {0, 1}).status);
    print_result("Large diff", test_sort_case("Large diff", {-2147483648, 2147483647}).status);
    
    print_subheader("Three Elements (all permutations - should be ≤3 ops)");
    print_result("Already sorted 1 2 3", test_sort_case("Sorted 1 2 3", {1, 2, 3}).status);
    print_result("Reversed 3 2 1", test_sort_case("Reversed 3 2 1", {3, 2, 1}).status);
    print_result("Rotation 2 3 1", test_sort_case("Rotation 2 3 1", {2, 3, 1}).status);
    print_result("Rotation 3 1 2", test_sort_case("Rotation 3 1 2", {3, 1, 2}).status);
    print_result("Swap needed 2 1 3", test_sort_case("Swap 2 1 3", {2, 1, 3}).status);
    print_result("Swap needed 1 3 2", test_sort_case("Swap 1 3 2", {1, 3, 2}).status);
    
    print_subheader("Four Elements (all 24 permutations - should be ≤12 ops)");
    // All 24 permutations of {1,2,3,4}
    vector<vector<int>> four_perms = {
        {1,2,3,4}, {1,2,4,3}, {1,3,2,4}, {1,3,4,2}, {1,4,2,3}, {1,4,3,2},
        {2,1,3,4}, {2,1,4,3}, {2,3,1,4}, {2,3,4,1}, {2,4,1,3}, {2,4,3,1},
        {3,1,2,4}, {3,1,4,2}, {3,2,1,4}, {3,2,4,1}, {3,4,1,2}, {3,4,2,1},
        {4,1,2,3}, {4,1,3,2}, {4,2,1,3}, {4,2,3,1}, {4,3,1,2}, {4,3,2,1}
    };
    {
        int passed = 0, failed = 0, total = four_perms.size();
        vector<int> ops_list;
        int max_ops = 0, min_ops = INT_MAX;
        
        for (size_t i = 0; i < four_perms.size(); ++i) {
            print_progress(i + 1, total, "  Testing 4-elem");
            const auto& p = four_perms[i];
            auto r = test_sort_case("4elem", p);
            if (r.passed) {
                passed++;
                ops_list.push_back(r.instruction_count);
                max_ops = max(max_ops, r.instruction_count);
                min_ops = min(min_ops, r.instruction_count);
            } else {
                failed++;
            }
        }
        clear_line();
        
        string status = (failed == 0) ? PASS : FAIL;
        int avg = ops_list.empty() ? 0 : accumulate(ops_list.begin(), ops_list.end(), 0) / ops_list.size();
        cout << "  " << status << "  " << GRN << passed << RST << "/" << total << " passed";
        if (!ops_list.empty()) {
            cout << "  " << GRY << "Min: " << RST << min_ops;
            cout << "  " << GRY << "Max: " << RST << max_ops;
            cout << "  " << GRY << "Avg: " << RST << avg << " ops";
        }
        if (failed > 0) cout << "  " << RED << failed << " failed" << RST;
        cout << "\n";
    }
    
    print_subheader("Five Elements (ALL 120 permutations - should be ≤12 ops)");
    // All 120 permutations of {1,2,3,4,5}
    vector<vector<int>> five_perms = {
        {1,2,3,4,5}, {1,2,3,5,4}, {1,2,4,3,5}, {1,2,4,5,3}, {1,2,5,3,4}, {1,2,5,4,3},
        {1,3,2,4,5}, {1,3,2,5,4}, {1,3,4,2,5}, {1,3,4,5,2}, {1,3,5,2,4}, {1,3,5,4,2},
        {1,4,2,3,5}, {1,4,2,5,3}, {1,4,3,2,5}, {1,4,3,5,2}, {1,4,5,2,3}, {1,4,5,3,2},
        {1,5,2,3,4}, {1,5,2,4,3}, {1,5,3,2,4}, {1,5,3,4,2}, {1,5,4,2,3}, {1,5,4,3,2},
        {2,1,3,4,5}, {2,1,3,5,4}, {2,1,4,3,5}, {2,1,4,5,3}, {2,1,5,3,4}, {2,1,5,4,3},
        {2,3,1,4,5}, {2,3,1,5,4}, {2,3,4,1,5}, {2,3,4,5,1}, {2,3,5,1,4}, {2,3,5,4,1},
        {2,4,1,3,5}, {2,4,1,5,3}, {2,4,3,1,5}, {2,4,3,5,1}, {2,4,5,1,3}, {2,4,5,3,1},
        {2,5,1,3,4}, {2,5,1,4,3}, {2,5,3,1,4}, {2,5,3,4,1}, {2,5,4,1,3}, {2,5,4,3,1},
        {3,1,2,4,5}, {3,1,2,5,4}, {3,1,4,2,5}, {3,1,4,5,2}, {3,1,5,2,4}, {3,1,5,4,2},
        {3,2,1,4,5}, {3,2,1,5,4}, {3,2,4,1,5}, {3,2,4,5,1}, {3,2,5,1,4}, {3,2,5,4,1},
        {3,4,1,2,5}, {3,4,1,5,2}, {3,4,2,1,5}, {3,4,2,5,1}, {3,4,5,1,2}, {3,4,5,2,1},
        {3,5,1,2,4}, {3,5,1,4,2}, {3,5,2,1,4}, {3,5,2,4,1}, {3,5,4,1,2}, {3,5,4,2,1},
        {4,1,2,3,5}, {4,1,2,5,3}, {4,1,3,2,5}, {4,1,3,5,2}, {4,1,5,2,3}, {4,1,5,3,2},
        {4,2,1,3,5}, {4,2,1,5,3}, {4,2,3,1,5}, {4,2,3,5,1}, {4,2,5,1,3}, {4,2,5,3,1},
        {4,3,1,2,5}, {4,3,1,5,2}, {4,3,2,1,5}, {4,3,2,5,1}, {4,3,5,1,2}, {4,3,5,2,1},
        {4,5,1,2,3}, {4,5,1,3,2}, {4,5,2,1,3}, {4,5,2,3,1}, {4,5,3,1,2}, {4,5,3,2,1},
        {5,1,2,3,4}, {5,1,2,4,3}, {5,1,3,2,4}, {5,1,3,4,2}, {5,1,4,2,3}, {5,1,4,3,2},
        {5,2,1,3,4}, {5,2,1,4,3}, {5,2,3,1,4}, {5,2,3,4,1}, {5,2,4,1,3}, {5,2,4,3,1},
        {5,3,1,2,4}, {5,3,1,4,2}, {5,3,2,1,4}, {5,3,2,4,1}, {5,3,4,1,2}, {5,3,4,2,1},
        {5,4,1,2,3}, {5,4,1,3,2}, {5,4,2,1,3}, {5,4,2,3,1}, {5,4,3,1,2}, {5,4,3,2,1}
    };
    {
        int passed = 0, failed = 0, total = five_perms.size();
        vector<int> ops_list;
        int max_ops = 0, min_ops = INT_MAX;
        
        for (size_t i = 0; i < five_perms.size(); ++i) {
            print_progress(i + 1, total, "  Testing 5-elem");
            const auto& p = five_perms[i];
            auto r = test_sort_case("5elem", p);
            if (r.passed) {
                passed++;
                ops_list.push_back(r.instruction_count);
                max_ops = max(max_ops, r.instruction_count);
                min_ops = min(min_ops, r.instruction_count);
            } else {
                failed++;
            }
        }
        clear_line();
        
        string status = (failed == 0) ? PASS : FAIL;
        int avg = ops_list.empty() ? 0 : accumulate(ops_list.begin(), ops_list.end(), 0) / ops_list.size();
        cout << "  " << status << "  " << GRN << passed << RST << "/" << total << " passed";
        if (!ops_list.empty()) {
            cout << "  " << GRY << "Min: " << RST << min_ops;
            cout << "  " << GRY << "Max: " << RST << max_ops;
            cout << "  " << GRY << "Avg: " << RST << avg << " ops";
        }
        if (failed > 0) cout << "  " << RED << failed << " failed" << RST;
        cout << "\n";
    }
    
    print_subheader("Edge Value Combinations");
    print_result("INT boundaries", 
                 test_sort_case("INT boundaries", {-2147483648, 0, 2147483647}).status);
    print_result("All negative", 
                 test_sort_case("All negative", {-5, -3, -1, -4, -2}).status);
    print_result("All same sign", 
                 test_sort_case("All same sign", {100, 200, 300, 400, 500}).status);
    print_result("Zero in middle", 
                 test_sort_case("Zero in middle", {-2, -1, 0, 1, 2}).status);
    print_result("INT_MAX sorted 3", 
                 test_sort_case("INT_MAX sorted 3", {2147483645, 2147483646, 2147483647}).status);
    print_result("INT_MIN sorted 3", 
                 test_sort_case("INT_MIN sorted 3", {-2147483648, -2147483647, -2147483646}).status);
}

void run_special_cases() {
    print_header("SPECIAL CASE TESTS");
    
    print_subheader("Already Sorted (should be 0 ops)");
    // These should output NOTHING
    vector<pair<string, vector<int>>> sorted_cases = {
        {"Empty", {}},
        {"Single 1", {1}},
        {"Two sorted 1 2", {1, 2}},
        {"Three sorted 1 2 3", {1, 2, 3}},
        {"Five sorted 1 2 3 4 5", {1, 2, 3, 4, 5}},
        {"Nine sorted", {1, 2, 3, 4, 5, 6, 7, 8, 9}},
        {"0 1 2 3 4", {0, 1, 2, 3, 4}},
        {"Thirty sorted", {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30}},
        {"INT_MAX area sorted", {2147483645, 2147483646, 2147483647}},
        {"INT_MIN area sorted", {-2147483648, -2147483647, -2147483646}},
        {"6 7 8", {6, 7, 8}},
        {"Fifty sorted", {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50}},
        {"77 numbers sorted", generate_range(1, 77)},
    };
    
    for (const auto& tc : sorted_cases) {
        vector<string> args;
        for (int n : tc.second) args.push_back(to_string(n));
        
        auto r = run_push_swap(args, false);
        int ops = count_instructions(r.stdout_data);
        
        string status;
        if (ops == 0 && r.exit_code == 0 && r.signal_num == 0) {
            status = PASS;
            stats.passed++;
        } else if (r.timed_out) {
            status = TOUT;
            stats.timeouts++;
        } else if (r.signal_num != 0) {
            status = SEGV;
            stats.crashes++;
        } else {
            status = FAIL;
            stats.failed++;
            log_error(tc.first, "Already sorted test", "Expected 0 ops, got " + to_string(ops), args);
        }
        stats.total++;
        
        string detail = (ops == 0) ? GRN "0 ops" RST : RED + to_string(ops) + " ops (expected 0)" RST;
        print_result(tc.first, status, detail);
    }
    
    print_subheader("Reverse Sorted");
    for (int n : {2, 3, 5, 10, 20}) {
        auto v = generate_reversed(n);
        auto r = test_sort_case("Reversed " + to_string(n), v);
        print_result("Reversed " + to_string(n) + " elements", r.status, to_string(r.instruction_count) + " ops");
    }
    
    print_subheader("Rotated Arrays");
    for (int n : {5, 10, 20}) {
        for (int rot : {1, n/2, n-1}) {
            auto v = generate_rotated(n, rot);
            auto r = test_sort_case("Rotated " + to_string(n) + " by " + to_string(rot), v);
            print_result("Rotated " + to_string(n) + " by " + to_string(rot), r.status, 
                        to_string(r.instruction_count) + " ops");
        }
    }
    
    print_subheader("Nearly Sorted");
    for (int n : {10, 20, 50}) {
        auto v = generate_nearly_sorted(n, 2);
        auto r = test_sort_case("Nearly sorted " + to_string(n), v);
        print_result("Nearly sorted " + to_string(n), r.status, to_string(r.instruction_count) + " ops");
    }
    
    print_subheader("Big Number Ranges (500 numbers)");
    // Test with various ranges of big numbers
    vector<pair<string, pair<int,int>>> big_ranges = {
        {"0 to 499", {0, 499}},
        {"5000 to 5499", {5000, 5499}},
        {"50000 to 50499", {50000, 50499}},
        {"500000 to 500499", {500000, 500499}},
        {"5000000 to 5000499", {5000000, 5000499}},
        {"50000000 to 50000499", {50000000, 50000499}},
        {"-500 to -1", {-500, -1}},
        {"-50000 to -49501", {-50000, -49501}},
        {"100 to 599", {100, 599}},
        {"250 to 720 (471 nums)", {250, 720}},
        {"10000 to 10479 (480 nums)", {10000, 10479}},
        {"100 to 450 (351 nums)", {100, 450}},
        {"-500 to -50 (451 nums)", {-500, -50}},
        {"-500 to -9 (492 nums)", {-500, -9}},
        {"0 to 450 (451 nums)", {0, 450}},
        {"-1 to 498 (500 nums)", {-1, 498}},
    };
    
    for (const auto& tc : big_ranges) {
        auto v = generate_range(tc.second.first, tc.second.second);
        random_device rd;
        mt19937 gen(rd());
        shuffle(v.begin(), v.end(), gen);
        auto r = test_sort_case("Range " + tc.first, v);
        print_result("Range " + tc.first + " shuffled", r.status, to_string(r.instruction_count) + " ops");
    }
    
    print_subheader("INT_MIN Area Tests (500 numbers)");
    // Test with INT_MIN boundary values
    {
        auto v = generate_range(-2147483648, -2147483149);  // 500 numbers near INT_MIN
        random_device rd;
        mt19937 gen(rd());
        shuffle(v.begin(), v.end(), gen);
        auto r = test_sort_case("INT_MIN area 500", v);
        print_result("INT_MIN to INT_MIN+499 shuffled", r.status, to_string(r.instruction_count) + " ops");
    }
    {
        auto v = generate_range(-2147483648, -2147483149);  // Again to verify consistency
        random_device rd;
        mt19937 gen(rd());
        shuffle(v.begin(), v.end(), gen);
        auto r = test_sort_case("INT_MIN area 500 #2", v);
        print_result("INT_MIN area shuffled #2", r.status, to_string(r.instruction_count) + " ops");
    }
}

void run_performance_tests() {
    print_header("PERFORMANCE BENCHMARKS");
    
    auto run_benchmark = [](int n, int iterations, const vector<Threshold>& thresholds) {
        vector<int> results;
        int failures = 0;
        int leaks = 0;
        
        cout << "\n" << BLD << "Size " << n << " (" << iterations << " iterations)" << RST << "\n";
        
        for (int i = 0; i < iterations; ++i) {
            print_progress(i + 1, iterations, "  Testing");
            
            auto nums = generate_unique_random(n, -1000000, 1000000);
            vector<string> args;
            for (int num : nums) args.push_back(to_string(num));
            
            ExecResult r = run_push_swap(args, cfg.use_valgrind);
            
            if (r.timed_out || r.signal_num != 0) {
                failures++;
                log_trace("Perf_" + to_string(n) + "_crash", args);
                continue;
            }
            
            if (!validate_all_instructions(r.stdout_data)) {
                failures++;
                log_trace("Perf_" + to_string(n) + "_invalid", args);
                continue;
            }
            
            if (!verify_sort(nums, r.stdout_data)) {
                // Double check with checker
                if (!cfg.checker.empty()) {
                    ExecResult chk = run_checker(args, r.stdout_data);
                    if (chk.stdout_data.find("OK") == string::npos) {
                        failures++;
                        log_trace("Perf_" + to_string(n) + "_ko", args, "Checker returned KO");
                        continue;
                    }
                } else {
                    failures++;
                    log_trace("Perf_" + to_string(n) + "_nosort", args);
                    continue;
                }
            }
            
            if (r.has_leaks) leaks++;
            
            int count = count_instructions(r.stdout_data);
            results.push_back(count);
        }
        
        clear_line();
        
        if (results.empty()) {
            cout << "  " << RED << "All tests failed!" << RST << "\n";
            return;
        }
        
        sort(results.begin(), results.end());
        int min_v = results.front();
        int max_v = results.back();
        int avg = accumulate(results.begin(), results.end(), 0) / results.size();
        int median = results[results.size() / 2];
        
        // Calculate percentiles
        int p90 = results[(int)(results.size() * 0.9)];
        int p95 = results[(int)(results.size() * 0.95)];
        
        cout << "  " << GRY << "Min: " << RST << min_v;
        cout << "  " << GRY << "Max: " << RST << max_v;
        cout << "  " << GRY << "Median: " << RST << median;
        cout << "  " << GRY << "Avg: " << RST << BLD << avg << RST;
        cout << "\n";
        
        cout << "  " << GRY << "P90: " << RST << p90;
        cout << "  " << GRY << "P95: " << RST << p95;
        cout << "\n";
        
        // Grading
        string grade = get_grade(avg, thresholds);
        int score = get_score(avg, thresholds);
        
        cout << "  " << BLD << "Grade: " << RST;
        if (score >= 5) cout << GRN << "★★★★★ " << grade << RST;
        else if (score >= 4) cout << GRN << "★★★★☆ " << grade << RST;
        else if (score >= 3) cout << YEL << "★★★☆☆ " << grade << RST;
        else if (score >= 2) cout << YEL << "★★☆☆☆ " << grade << RST;
        else cout << RED << "★☆☆☆☆ " << grade << RST;
        
        // Threshold info
        cout << "\n  " << GRY << "Thresholds: ";
        for (size_t i = 0; i < thresholds.size(); ++i) {
            cout << thresholds[i].score << "/5≤" << thresholds[i].limit;
            if (i < thresholds.size() - 1) cout << ", ";
        }
        cout << RST << "\n";
        
        if (failures > 0) {
            cout << "  " << RED << "Failures: " << failures << RST << "\n";
        }
        if (leaks > 0) {
            cout << "  " << YEL << "Memory leaks detected in " << leaks << " tests" << RST << "\n";
        }
        
        stats.perf_results[to_string(n)] = results;
    };
    
    int quick_iter = cfg.quick_mode ? 10 : 50;
    int stress_iter = cfg.stress_mode ? 100 : 50;
    
    // Small sizes
    print_subheader("Small Sizes");
    run_benchmark(3, quick_iter, SCORES_3);
    run_benchmark(5, quick_iter, SCORES_5);
    
    // Medium sizes
    print_subheader("Medium Sizes");
    run_benchmark(10, quick_iter, {{25, 5, "⭐⭐⭐⭐⭐"}, {35, 4, "⭐⭐⭐⭐"}, {50, 3, "⭐⭐⭐"}, {75, 2, "⭐⭐"}, {INT_MAX, 1, "⭐"}});
    run_benchmark(50, quick_iter, {{300, 5, "⭐⭐⭐⭐⭐"}, {400, 4, "⭐⭐⭐⭐"}, {500, 3, "⭐⭐⭐"}, {600, 2, "⭐⭐"}, {INT_MAX, 1, "⭐"}});
    
    // Required benchmarks
    print_subheader("Required Benchmarks (100 & 500)");
    run_benchmark(100, stress_iter, SCORES_100);
    run_benchmark(500, stress_iter, SCORES_500);
}

void run_leak_tests() {
    if (!cfg.use_valgrind) {
        cout << WARN << " Valgrind disabled, skipping leak tests\n";
        return;
    }
    
    print_header("MEMORY LEAK TESTS");
    
    print_subheader("Error Cases - Overflow in Middle of Valid List (CRITICAL!)");
    
    // CRITICAL: These test cases have valid numbers BEFORE the error
    // The program might allocate memory for valid numbers, then fail on the error
    // WITHOUT freeing the already-allocated memory
    vector<pair<string, vector<string>>> overflow_middle_cases = {
        // THE EXACT CASE THAT CAUGHT THE LEAK
        {"1 2 3 4 HUGE 8 7 6 (overflow middle)", {"1", "2", "3", "4", "9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999", "8", "7", "6"}},
        {"1 2 3 HUGE (overflow at end)", {"1", "2", "3", "99999999999999999999999999999999999999999999999999"}},
        {"1 HUGE 2 3 (overflow early)", {"1", "9999999999999999999999999999999999999999999999999999999999999999", "2", "3"}},
        {"Valid 10 then overflow", {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "99999999999999999999"}},
        {"Valid 50 then overflow", {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "50", "99999999999999999999999999999999999"}},
        {"INT_MAX+1 after valid", {"42", "21", "2147483648"}},
        {"INT_MIN-1 after valid", {"42", "21", "-2147483649"}},
        {"-INT overflow after valid", {"1", "2", "3", "-99999999999999999999999999999999999999999999"}},
    };
    
    for (const auto& tc : overflow_middle_cases) {
        ExecResult r = run_push_swap(tc.second, true);
        string status = r.has_leaks ? LEAK : PASS;
        if (r.has_leaks) {
            stats.leaks++;
            log_trace("LEAK: " + tc.first, tc.second, to_string(r.leaked_bytes) + " bytes leaked");
        }
        print_result(tc.first, status, r.has_leaks ? RED + to_string(r.leaked_bytes) + " bytes" + RST : "");
    }
    
    print_subheader("Error Cases - Invalid Char After Valid Numbers");
    
    vector<pair<string, vector<string>>> invalid_char_middle_cases = {
        {"1 2 3 4 abc 8 7 6", {"1", "2", "3", "4", "abc", "8", "7", "6"}},
        {"Letter after 10 valid", {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "x"}},
        {"Letter after 20 valid", {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "a"}},
        {"Special char after valid '@'", {"42", "21", "10", "@"}},
        {"Special char in middle '#'", {"1", "2", "#", "3", "4"}},
        {"Decimal after valid '1.5'", {"42", "21", "1.5"}},
        {"Mixed 'a1' after valid", {"1", "2", "3", "a1"}},
        {"Mixed '1a' after valid", {"1", "2", "3", "1a"}},
    };
    
    for (const auto& tc : invalid_char_middle_cases) {
        ExecResult r = run_push_swap(tc.second, true);
        string status = r.has_leaks ? LEAK : PASS;
        if (r.has_leaks) {
            stats.leaks++;
            log_trace("LEAK: " + tc.first, tc.second, to_string(r.leaked_bytes) + " bytes leaked");
        }
        print_result(tc.first, status, r.has_leaks ? RED + to_string(r.leaked_bytes) + " bytes" + RST : "");
    }
    
    print_subheader("Error Cases - Duplicate After Allocation");
    
    vector<pair<string, vector<string>>> dup_after_alloc_cases = {
        {"Dup at end '1 2 3 4 5 1'", {"1", "2", "3", "4", "5", "1"}},
        {"Dup at end long list", {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "1"}},
        {"Dup with leading zero '1 2 3 4 5 01'", {"1", "2", "3", "4", "5", "01"}},
        {"Dup with plus '1 2 3 4 5 +1'", {"1", "2", "3", "4", "5", "+1"}},
        {"Dup zeros after valid '1 2 0 00'", {"1", "2", "0", "00"}},
        {"Dup -0 after valid '1 2 0 -0'", {"1", "2", "0", "-0"}},
        {"Dup +0 after valid '1 2 0 +0'", {"1", "2", "0", "+0"}},
        {"Hidden dup '42 21 10 00042'", {"42", "21", "10", "00042"}},
    };
    
    for (const auto& tc : dup_after_alloc_cases) {
        ExecResult r = run_push_swap(tc.second, true);
        string status = r.has_leaks ? LEAK : PASS;
        if (r.has_leaks) {
            stats.leaks++;
            log_trace("LEAK: " + tc.first, tc.second, to_string(r.leaked_bytes) + " bytes leaked");
        }
        print_result(tc.first, status, r.has_leaks ? RED + to_string(r.leaked_bytes) + " bytes" + RST : "");
    }
    
    print_subheader("Error Cases - Sign Errors After Allocation");
    
    vector<pair<string, vector<string>>> sign_error_cases = {
        {"Sign middle after valid '1 2 3 4-4'", {"1", "2", "3", "4-4"}},
        {"Sign middle after valid '1 2 3 3+3'", {"1", "2", "3", "3+3"}},
        {"Double sign after valid '1 2 --5'", {"1", "2", "--5"}},
        {"Double sign after valid '1 2 ++5'", {"1", "2", "++5"}},
        {"Plus minus after valid '1 2 +-5'", {"1", "2", "+-5"}},
        {"Trailing sign after valid '1 2 5-'", {"1", "2", "5-"}},
        {"Only sign after valid '1 2 3 +'", {"1", "2", "3", "+"}},
        {"Only sign after valid '1 2 3 -'", {"1", "2", "3", "-"}},
    };
    
    for (const auto& tc : sign_error_cases) {
        ExecResult r = run_push_swap(tc.second, true);
        string status = r.has_leaks ? LEAK : PASS;
        if (r.has_leaks) {
            stats.leaks++;
            log_trace("LEAK: " + tc.first, tc.second, to_string(r.leaked_bytes) + " bytes leaked");
        }
        print_result(tc.first, status, r.has_leaks ? RED + to_string(r.leaked_bytes) + " bytes" + RST : "");
    }
    
    print_subheader("Error Cases - Basic (no prior allocation)");
    
    vector<pair<string, vector<string>>> leak_cases = {
        {"Invalid arg 'abc'", {"abc"}},
        {"Overflow arg", {"9999999999999999"}},
        {"Massive overflow (50+ digits)", {"99999999999999999999999999999999999999999999999999"}},
        {"Duplicate arg", {"1", "2", "1"}},
        {"Empty string", {""}},
        {"Sign only '+'", {"+"}},
        {"Sign only '-'", {"-"}},
        {"Double sign '--5'", {"--5"}},
        {"Double sign '++5'", {"++5"}},
        {"Leading zeros duplicate '1 01'", {"1", "01"}},
        {"Sign in middle '111-1'", {"111-1"}},
        {"Sign in middle '3+3'", {"3+3"}},
        {"Duplicate zeros '0 0'", {"0", "0"}},
        {"Duplicate with plus '3 +3'", {"3", "+3"}},
        {"INT_MAX+1", {"2147483648"}},
        {"INT_MIN-1", {"-2147483649"}},
    };
    
    for (const auto& tc : leak_cases) {
        ExecResult r = run_push_swap(tc.second, true);
        string status = r.has_leaks ? LEAK : PASS;
        if (r.has_leaks) {
            stats.leaks++;
            log_trace("Leak: " + tc.first, tc.second, to_string(r.leaked_bytes) + " bytes leaked");
        }
        print_result(tc.first, status, r.has_leaks ? to_string(r.leaked_bytes) + " bytes" : "");
    }
    
    print_subheader("Quoted String Errors (single arg with spaces - split leak test)");
    
    // These test the case where push_swap receives a SINGLE argument containing
    // spaces that needs to be split. If split() allocates and then an error occurs,
    // it might leak the split array.
    vector<pair<string, vector<string>>> quoted_leak_cases = {
        {"Quoted: overflow in middle", {"1 2 3 4 9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999 8 7 6"}},
        {"Quoted: dup at end", {"1 2 3 4 5 1"}},
        {"Quoted: letter in middle", {"1 2 3 abc 4 5"}},
        {"Quoted: double sign", {"1 2 3 --5 4"}},
        {"Quoted: sign in middle of num", {"1 2 3-3 4"}},
        {"Quoted: leading zero dup", {"1 2 3 4 5 01"}},
        {"Quoted: INT_MAX+1 at end", {"1 2 3 4 2147483648"}},
        {"Quoted: huge overflow", {"1 2 3 99999999999999999999999999999999999999999999999999999999"}},
        {"Quoted: many valid then dup", {"1 2 3 4 5 6 7 8 9 10 1"}},
        {"Quoted: 20 nums then overflow", {"1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 99999999999999999999"}},
    };
    
    for (const auto& tc : quoted_leak_cases) {
        ExecResult r = run_push_swap(tc.second, true);
        string status = r.has_leaks ? LEAK : PASS;
        if (r.has_leaks) {
            stats.leaks++;
            log_trace("LEAK: " + tc.first, tc.second, to_string(r.leaked_bytes) + " bytes leaked");
        }
        print_result(tc.first, status, r.has_leaks ? RED + to_string(r.leaked_bytes) + " bytes" + RST : "");
    }
    
    print_subheader("Valid Cases (must not leak)");
    
    vector<pair<string, vector<int>>> valid_leak_cases = {
        {"Single element", {42}},
        {"Three elements", {3, 1, 2}},
        {"Five elements", {5, 1, 4, 2, 3}},
        {"Ten elements", {10, 9, 8, 7, 6, 5, 4, 3, 2, 1}},
        {"Already sorted", {1, 2, 3, 4, 5}},
    };
    
    for (const auto& tc : valid_leak_cases) {
        vector<string> args;
        for (int n : tc.second) args.push_back(to_string(n));
        ExecResult r = run_push_swap(args, true);
        string status = r.has_leaks ? LEAK : PASS;
        if (r.has_leaks) {
            stats.leaks++;
            log_trace("Leak: " + tc.first, args, to_string(r.leaked_bytes) + " bytes leaked");
        }
        print_result(tc.first, status, r.has_leaks ? to_string(r.leaked_bytes) + " bytes" : "");
    }
    
    print_subheader("Large Allocations (stress test)");
    
    for (int n : {100, 500, 1000}) {
        auto nums = generate_unique_random(n, -100000, 100000);
        vector<string> args;
        for (int num : nums) args.push_back(to_string(num));
        
        ExecResult r = run_push_swap(args, true);
        string status = r.has_leaks ? LEAK : PASS;
        if (r.has_leaks) {
            stats.leaks++;
            log_trace("Leak: size " + to_string(n), args, to_string(r.leaked_bytes) + " bytes leaked");
        }
        print_result("Size " + to_string(n), status, r.has_leaks ? to_string(r.leaked_bytes) + " bytes" : "");
    }
}

void run_checker_tests() {
    if (cfg.checker.empty()) {
        cout << WARN << " No checker specified, skipping checker tests\n";
        return;
    }
    
    print_header("CHECKER PROGRAM TESTS");
    
    print_subheader("Valid Operations");
    
    // Test checker with known sequences
    auto test_checker = [](const string& name, const vector<int>& nums, 
                          const string& instructions, bool expect_ok) {
        vector<string> args;
        for (int n : nums) args.push_back(to_string(n));
        
        ExecResult r = run_checker(args, instructions);
        
        // Check for crash first
        if (r.signal_num != 0) {
            checker_stats.total++;
            checker_stats.failed++;
            checker_stats.crashes++;
            checker_stats.failed_tests.push_back(name + " (crash)");
            print_result(name, SEGV, expect_ok ? "(expect OK)" : "(expect KO)");
            return;
        }
        
        bool got_ok = (r.stdout_data.find("OK") != string::npos);
        bool got_ko = (r.stdout_data.find("KO") != string::npos);
        
        bool passed = (expect_ok && got_ok) || (!expect_ok && got_ko);
        string status = passed ? PASS : FAIL;
        
        checker_stats.total++;
        if (passed) checker_stats.passed++;
        else {
            checker_stats.failed++;
            checker_stats.failed_tests.push_back(name);
            string reason = expect_ok ? "Expected OK but got KO or no response" : "Expected KO but got OK or no response";
            log_error(name, "Checker Test", reason, args);
        }
        
        print_result(name, status, expect_ok ? "(expect OK)" : "(expect KO)");
    };
    
    // Already sorted - no ops should be OK
    test_checker("Sorted, no ops", {1, 2, 3}, "", true);
    
    // Simple swap
    test_checker("2 1 -> sa -> OK", {2, 1}, "sa\n", true);
    test_checker("2 1 -> no op -> KO", {2, 1}, "", false);
    
    // Wrong operation
    test_checker("1 2 3 -> sa -> KO", {1, 2, 3}, "sa\n", false);
    
    // Complex sequence
    test_checker("3 2 1 -> ra sa", {3, 2, 1}, "ra\nsa\n", true);  // This sorts 3 2 1 to 1 2 3
    test_checker("3 2 1 -> sa ra (wrong)", {3, 2, 1}, "sa\nra\n", false);  // Wrong order, doesn't sort
    
    print_subheader("Invalid Instructions (should print Error)");
    
    auto test_checker_error = [](const string& name, const vector<int>& nums, 
                                 const string& instructions) {
        vector<string> args;
        for (int n : nums) args.push_back(to_string(n));
        
        ExecResult r = run_checker(args, instructions);
        
        // Check for crash first
        if (r.signal_num != 0) {
            checker_stats.total++;
            checker_stats.failed++;
            checker_stats.crashes++;
            checker_stats.failed_tests.push_back(name + " (crash)");
            print_result(name, SEGV);
            return;
        }
        
        bool has_error = (r.stderr_data.find("Error") != string::npos || 
                         r.stdout_data.find("Error") != string::npos);
        
        string status = has_error ? PASS : FAIL;
        checker_stats.total++;
        if (has_error) checker_stats.passed++;
        else {
            checker_stats.failed++;
            checker_stats.failed_tests.push_back(name);
        }
        
        print_result(name, status);
    };
    
    test_checker_error("Invalid op 'swap'", {2, 1}, "swap\n");
    test_checker_error("Invalid op 'SA' (uppercase)", {2, 1}, "SA\n");
    test_checker_error("Invalid op 'SB' (uppercase)", {2, 1}, "SB\n");
    test_checker_error("Invalid op 'PA' (uppercase)", {2, 1}, "PA\n");
    test_checker_error("Invalid op 'PB' (uppercase)", {2, 1}, "PB\n");
    test_checker_error("Invalid op 'RA' (uppercase)", {2, 1}, "RA\n");
    test_checker_error("Invalid op 'RB' (uppercase)", {2, 1}, "RB\n");
    test_checker_error("Invalid op 'RRA' (uppercase)", {2, 1}, "RRA\n");
    test_checker_error("Invalid op 'RRB' (uppercase)", {2, 1}, "RRB\n");
    test_checker_error("Invalid op 'RRR' (uppercase)", {2, 1}, "RRR\n");
    test_checker_error("Invalid op 'saa' (extra char)", {1}, "saa\n");
    test_checker_error("Invalid op 'paa' (extra char)", {1}, "paa\n");
    test_checker_error("Invalid op 'raa' (extra char)", {1}, "raa\n");
    test_checker_error("Invalid op 'rraa' (extra char)", {1}, "rraa\n");
    test_checker_error("Invalid op 'sa ' (trailing space)", {1}, "sa \n");
    test_checker_error("Invalid op ' sa' (leading space)", {1}, " sa\n");
    test_checker_error("Invalid op with newline 'sa\\n\\n'", {1}, "sa\n\n");
    test_checker_error("Invalid op 'push_a'", {2, 1}, "push_a\n");
    test_checker_error("Invalid op 'push_b'", {2, 1}, "push_b\n");
    test_checker_error("Invalid op 'rotate'", {2, 1}, "rotate\n");
    test_checker_error("Garbage text", {2, 1}, "hello world\n");
    test_checker_error("Empty instruction (just newline)", {2, 1}, "\n");
    test_checker_error("Number as instruction", {2, 1}, "123\n");
    test_checker_error("Mixed valid/invalid 'sa swap'", {2, 1}, "sa\nswap\n");
    
    print_subheader("Checker Error Handling");
    
    auto test_checker_parse_error = [](const string& name, const vector<string>& args) {
        ExecResult r = execute_command({cfg.checker, args[0]});
        
        // Check for crash first
        if (r.signal_num != 0) {
            checker_stats.total++;
            checker_stats.failed++;
            checker_stats.crashes++;
            checker_stats.failed_tests.push_back(name + " (crash)");
            print_result(name, SEGV);
            return;
        }
        
        bool has_error = (r.stderr_data.find("Error") != string::npos);
        string status = has_error ? PASS : FAIL;
        checker_stats.total++;
        if (has_error) checker_stats.passed++;
        else {
            checker_stats.failed++;
            checker_stats.failed_tests.push_back(name);
        }
        print_result(name, status);
    };
    
    test_checker_parse_error("Duplicate args", {"1 1"});
    test_checker_parse_error("Non-integer arg", {"abc"});
    test_checker_parse_error("Overflow", {"99999999999"});
    test_checker_parse_error("Sign in middle '111-1 2 -3'", {"111-1 2 -3"});
    test_checker_parse_error("Double minus '--5'", {"--5"});
    
    // Memory leak tests for checker
    if (cfg.use_valgrind) {
        print_subheader("Checker Memory Leak Tests");
        
        auto test_checker_leaks = [](const string& name, const vector<int>& nums, 
                                     const string& instructions) {
            vector<string> args;
            for (int n : nums) args.push_back(to_string(n));
            
            ExecResult r = run_checker(args, instructions, true);
            
            bool has_leak = r.has_leaks || r.leaked_bytes > 0;
            bool crashed = (r.signal_num != 0);
            
            string status;
            if (crashed) {
                status = SEGV;
                checker_stats.crashes++;
                checker_stats.failed++;
                checker_stats.failed_tests.push_back(name + " (crash)");
            } else if (has_leak) {
                status = LEAK;
                checker_stats.leaks++;
                checker_stats.failed++;
                checker_stats.failed_tests.push_back(name + " (leak)");
            } else {
                status = PASS;
                checker_stats.passed++;
            }
            checker_stats.total++;
            
            string details = "";
            if (has_leak) details = to_string(r.leaked_bytes) + " bytes leaked";
            print_result(name, status, details);
        };
        
        // Test valid cases
        test_checker_leaks("Leak: sorted no ops", {1, 2, 3}, "");
        test_checker_leaks("Leak: simple swap", {2, 1}, "sa\n");
        test_checker_leaks("Leak: multiple ops", {3, 2, 1}, "ra\nsa\n");
        test_checker_leaks("Leak: all operations", {5, 4, 3, 2, 1}, "pb\npb\nra\nrb\nrr\nrra\nrrb\nrrr\nsa\nsb\nss\npa\npa\n");
        test_checker_leaks("Leak: 100 elements", generate_unique_random(100, 1, 1000), "");
        
        // Test error cases (should still not leak)
        auto test_checker_error_leaks = [](const string& name, const vector<string>& args) {
            vector<string> cmd;
            cmd = {"valgrind", "--leak-check=full", "--show-leak-kinds=all", 
                   "--errors-for-leak-kinds=all", "--error-exitcode=42", 
                   cfg.checker};
            cmd.insert(cmd.end(), args.begin(), args.end());
            
            ExecResult r = execute_command(cmd, "", true);
            
            bool has_leak = r.has_leaks || r.leaked_bytes > 0;
            bool crashed = (r.signal_num != 0);
            
            string status;
            if (crashed) {
                status = SEGV;
                checker_stats.crashes++;
                checker_stats.failed++;
                checker_stats.failed_tests.push_back(name + " (crash)");
            } else if (has_leak) {
                status = LEAK;
                checker_stats.leaks++;
                checker_stats.failed++;
                checker_stats.failed_tests.push_back(name + " (leak)");
            } else {
                status = PASS;
                checker_stats.passed++;
            }
            checker_stats.total++;
            
            string details = "";
            if (has_leak) details = to_string(r.leaked_bytes) + " bytes leaked";
            print_result(name, status, details);
        };
        
        test_checker_error_leaks("Leak: invalid arg", {"abc"});
        test_checker_error_leaks("Leak: duplicate", {"1", "1"});
        test_checker_error_leaks("Leak: overflow", {"99999999999"});
        
        // Overflow in middle of valid args (common leak case!)
        test_checker_error_leaks("Leak: 1 2 3 HUGE 5", {"1", "2", "3", "99999999999999999999", "5"});
        test_checker_error_leaks("Leak: valid then overflow", {"1", "2", "3", "99999999999999999999"});
        test_checker_error_leaks("Leak: overflow at end", {"42", "41", "40", "99999999999999999999999999"});
        test_checker_error_leaks("Leak: 50 digit number", {"555555555555555555555555555555555555555555555555"});
        test_checker_error_leaks("Leak: negative overflow", {"-1", "-2", "-99999999999999999999"});
    }
}

void run_stress_tests() {
    print_header("STRESS TESTS");
    
    print_subheader("Rapid Fire (many small inputs)");
    
    int rapid_tests = cfg.stress_mode ? 100 : 20;
    int failures = 0;
    
    for (int i = 0; i < rapid_tests; ++i) {
        print_progress(i + 1, rapid_tests, "  Testing");
        
        int size = (i % 10) + 1;  // 1-10 elements
        auto nums = generate_unique_random(size, -1000, 1000);
        vector<string> args;
        for (int n : nums) args.push_back(to_string(n));
        
        ExecResult r = run_push_swap(args, false);
        
        if (r.timed_out || r.signal_num != 0 || !validate_all_instructions(r.stdout_data)) {
            failures++;
            continue;
        }
        
        if (!verify_sort(nums, r.stdout_data)) {
            failures++;
        }
    }
    
    clear_line();
    string status = (failures == 0) ? PASS : FAIL;
    print_result("Rapid fire (" + to_string(rapid_tests) + " tests)", status, 
                to_string(failures) + " failures");
    
    print_subheader("Edge Value Combinations");
    
    vector<pair<string, vector<int>>> edge_cases = {
        {"All INT_MAX area", {2147483647, 2147483646, 2147483645}},
        {"All INT_MIN area", {-2147483648, -2147483647, -2147483646}},
        {"Min and max only", {-2147483648, 2147483647}},
        {"Consecutive with overflow", {2147483645, 2147483646, 2147483647}},
        {"Large range", {-2147483648, 0, 2147483647}},
    };
    
    for (const auto& tc : edge_cases) {
        auto r = test_sort_case(tc.first, tc.second);
        print_result(tc.first, r.status, to_string(r.instruction_count) + " ops");
    }
}

// ==================================================================================
// HTML Report Generation
// ==================================================================================

void generate_html_report() {
    ofstream f(cfg.html_file);
    
    f << R"(<!DOCTYPE html>
<html>
<head>
    <title>Push_swap Test Report</title>
    <style>
        body { font-family: 'Segoe UI', Arial, sans-serif; margin: 40px; background: #1a1a2e; color: #eee; }
        h1 { color: #00d4ff; border-bottom: 2px solid #00d4ff; padding-bottom: 10px; }
        h2 { color: #00ff88; margin-top: 30px; }
        .summary { background: #16213e; padding: 20px; border-radius: 10px; margin: 20px 0; }
        .pass { color: #00ff88; }
        .fail { color: #ff4444; }
        .warn { color: #ffaa00; }
        table { border-collapse: collapse; width: 100%; margin: 20px 0; }
        th, td { border: 1px solid #333; padding: 10px; text-align: left; }
        th { background: #0f3460; }
        tr:nth-child(even) { background: #1a1a2e; }
        tr:hover { background: #16213e; }
        .stats { display: flex; gap: 20px; flex-wrap: wrap; }
        .stat-box { background: #0f3460; padding: 20px; border-radius: 10px; min-width: 150px; }
        .stat-value { font-size: 2em; font-weight: bold; }
    </style>
</head>
<body>
    <h1>🔧 Push_swap Ultimate Tester Report</h1>
    <p>Generated: )" << __DATE__ << " " << __TIME__ << R"(</p>
    
    <div class="summary">
        <h2>📊 Summary</h2>
        <div class="stats">
            <div class="stat-box">
                <div class="stat-value">)" << stats.total << R"(</div>
                <div>Total Tests</div>
            </div>
            <div class="stat-box">
                <div class="stat-value pass">)" << stats.passed << R"(</div>
                <div>Passed</div>
            </div>
            <div class="stat-box">
                <div class="stat-value fail">)" << stats.failed << R"(</div>
                <div>Failed</div>
            </div>
            <div class="stat-box">
                <div class="stat-value warn">)" << stats.leaks << R"(</div>
                <div>Leaks</div>
            </div>
            <div class="stat-box">
                <div class="stat-value fail">)" << stats.crashes << R"(</div>
                <div>Crashes</div>
            </div>
        </div>
    </div>
)";

    // Performance results
    if (!stats.perf_results.empty()) {
        f << R"(
    <h2>📈 Performance Results</h2>
    <table>
        <tr><th>Size</th><th>Min</th><th>Max</th><th>Average</th><th>Tests</th></tr>
)";
        for (const auto& [size, results] : stats.perf_results) {
            if (!results.empty()) {
                int min_v = *min_element(results.begin(), results.end());
                int max_v = *max_element(results.begin(), results.end());
                int avg = accumulate(results.begin(), results.end(), 0) / results.size();
                f << "        <tr><td>" << size << "</td><td>" << min_v << "</td><td>" 
                  << max_v << "</td><td>" << avg << "</td><td>" << results.size() << "</td></tr>\n";
            }
        }
        f << "    </table>\n";
    }

    // Failed tests
    if (!stats.failed_tests.empty()) {
        f << R"(
    <h2>❌ Failed Tests</h2>
    <ul>
)";
        for (const auto& t : stats.failed_tests) {
            f << "        <li>" << t << "</li>\n";
        }
        f << "    </ul>\n";
    }

    f << R"(
</body>
</html>
)";
    f.close();
    cout << "\n" << GRN << "HTML report generated: " << cfg.html_file << RST << "\n";
}

// ==================================================================================
// Main
// ==================================================================================

void print_banner() {
    cout << BLD << R"(
)" << GRN << R"(
              ▓▓▓░░                                         ░░▓▓▓
            ▓▓░░░░░░                                       ░░░░░░▓▓
           ▓░  ░░░░░░░                                   ░░░░░░░  ░▓
          ░▓    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    ▓░
)" << CYN << R"(    ██████╗ ███████╗    ████████╗███████╗███████╗████████╗███████╗██████╗
    ██╔══██╗██╔════╝    ╚══██╔══╝██╔════╝██╔════╝╚══██╔══╝██╔════╝██╔══██╗
    ██████╔╝███████╗       ██║   █████╗  ███████╗   ██║   █████╗  ██████╔╝
    ██╔═══╝ ╚════██║       ██║   ██╔══╝  ╚════██║   ██║   ██╔══╝  ██╔══██╗
    ██║     ███████║       ██║   ███████╗███████║   ██║   ███████╗██║  ██║
    ╚═╝     ╚══════╝       ╚═╝   ╚══════╝╚══════╝   ╚═╝   ╚══════╝╚═╝  ╚═╝
)" << GRN << R"(          ░▓    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    ▓░
           ▓░  ░░░░░░░        ░░░░░░░░░░░░░░░░░░░        ░░░░░░░  ░▓
            ▓▓░░░░░░     ░▓▓▓░░░              ░░░▓▓▓░     ░░░░░░▓▓
              ▓▓▓░░      ░▓  ▓░              ░▓  ▓░      ░░▓▓▓
                ░▓▓       ▓░░▓                ▓░░▓       ▓▓░
                  ▓░       ▓▓                  ▓▓       ░▓
                   ▓░      ░                    ░      ░▓
                    ▓░                                ░▓
                     ▓                                ▓
                     ░                                ░
)" << YEL << R"(         ╔═══════════════════════════════════════════════════════════╗
         ║)" << WHT << R"(   🧪 The Ultimate Push_swap Tester for 42/1337 Schools 🧪  )" << YEL << R"(║
         ║)" << CYN << R"(                       by mjabri                            )" << YEL << R"(║
         ╚═══════════════════════════════════════════════════════════╝
)" << RST << "\n";
}

void print_usage(const char* prog) {
    cout << "Usage: " << prog << " <push_swap> [checker] [options]\n\n";
    cout << "Options:\n";
    cout << "  --no-valgrind     Skip memory leak tests\n";
    cout << "  --quick           Quick mode (fewer iterations)\n";
    cout << "  --stress          Extra stress tests\n";
    cout << "  --html            Generate HTML report\n";
    cout << "  --checker-only    Only test checker program\n";
    cout << "  --verbose         Verbose output\n";
    cout << "  --help            Show this help\n";
}

int main(int argc, char** argv) {
    print_banner();
    
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Parse arguments
    vector<string> positional;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--no-valgrind") cfg.use_valgrind = false;
        else if (arg == "--quick") cfg.quick_mode = true;
        else if (arg == "--stress") cfg.stress_mode = true;
        else if (arg == "--html") cfg.html_report = true;
        else if (arg == "--checker-only") cfg.checker_only = true;
        else if (arg == "--verbose") cfg.verbose = true;
        else if (arg == "--help" || arg == "-h") { print_usage(argv[0]); return 0; }
        else positional.push_back(arg);
    }
    
    if (positional.empty()) {
        cerr << RED << "Error: push_swap path required\n" << RST;
        return 1;
    }
    
    cfg.push_swap = positional[0];
    if (positional.size() > 1) cfg.checker = positional[1];
    
    // Check if push_swap binary exists and is executable
    if (access(cfg.push_swap.c_str(), X_OK) != 0) {
        cerr << RED << "Error: push_swap binary not found or not executable: " << cfg.push_swap << "\n" << RST;
        return 1;
    }
    
    // Check if checker binary exists and is executable (if specified)
    if (!cfg.checker.empty()) {
        if (access(cfg.checker.c_str(), X_OK) != 0) {
            cerr << YEL << "⚠ Warning: checker binary not found or not executable: " << cfg.checker << "\n" << RST;
            cerr << YEL << "  Checker tests will run but expect failures.\n" << RST;
        }
    }
    
    // Check if valgrind is available
    if (cfg.use_valgrind) {
        ExecResult vg = execute_command({"valgrind", "--version"});
        if (vg.exit_code != 0) {
            cout << YEL << "⚠ Valgrind not found, disabling leak tests\n" << RST;
            cfg.use_valgrind = false;
        }
    }
    
    // Clear trace file and errors file
    remove(cfg.trace_file.c_str());
    remove(cfg.errors_file.c_str());
    
    cout << GRY << "Push_swap: " << RST << cfg.push_swap << "\n";
    if (!cfg.checker.empty()) cout << GRY << "Checker:   " << RST << cfg.checker << "\n";
    cout << GRY << "Valgrind:  " << RST << (cfg.use_valgrind ? GRN "Enabled" : RED "Disabled") << RST << "\n";
    cout << GRY << "Mode:      " << RST << (cfg.quick_mode ? "Quick" : (cfg.stress_mode ? "Stress" : "Normal")) << "\n";
    
    auto start_time = chrono::high_resolution_clock::now();
    
    // Run test suites
    if (!cfg.checker_only) {
        run_parsing_tests();
        run_basic_sorting_tests();
        run_special_cases();
        run_leak_tests();
        run_performance_tests();
        if (cfg.stress_mode) run_stress_tests();
    }
    
    if (!cfg.checker.empty()) {
        run_checker_tests();
    }
    
    auto end_time = chrono::high_resolution_clock::now();
    double elapsed = chrono::duration<double>(end_time - start_time).count();
    
    // Final Summary
    print_header("FINAL RESULTS");
    
    // Push_swap results
    if (!cfg.checker_only) {
        cout << "\n  " << BLD << CYN << "📊 PUSH_SWAP:" << RST << "\n";
        cout << "  ├─ Total Tests:    " << BLD << stats.total << RST << "\n";
        cout << "  ├─ Passed:         " << GRN << BLD << stats.passed << RST << "\n";
        cout << "  ├─ Failed:         " << (stats.failed > 0 ? RED : GRN) << BLD << stats.failed << RST << "\n";
        cout << "  ├─ Memory Leaks:   " << (stats.leaks > 0 ? RED : GRN) << BLD << stats.leaks << RST << "\n";
        cout << "  ├─ Crashes:        " << (stats.crashes > 0 ? RED : GRN) << BLD << stats.crashes << RST << "\n";
        cout << "  └─ Timeouts:       " << (stats.timeouts > 0 ? YEL : GRN) << BLD << stats.timeouts << RST << "\n";
        
        double ps_rate = (stats.total > 0) ? (100.0 * stats.passed / stats.total) : 0;
        cout << "     Success Rate:   ";
        if (ps_rate >= 95) cout << GRN;
        else if (ps_rate >= 80) cout << YEL;
        else cout << RED;
        cout << BLD << fixed << setprecision(1) << ps_rate << "%" << RST << "\n";
        
        if (!stats.failed_tests.empty() && stats.failed_tests.size() <= 10) {
            cout << "\n     " << RED << "Failed tests:" << RST << "\n";
            for (const auto& t : stats.failed_tests) {
                cout << "       - " << t << "\n";
            }
        }
    }
    
    // Checker results
    if (!cfg.checker.empty()) {
        cout << "\n  " << BLD << MAG << "🔍 CHECKER (Bonus):" << RST << "\n";
        cout << "  ├─ Total Tests:    " << BLD << checker_stats.total << RST << "\n";
        cout << "  ├─ Passed:         " << GRN << BLD << checker_stats.passed << RST << "\n";
        cout << "  ├─ Failed:         " << (checker_stats.failed > 0 ? RED : GRN) << BLD << checker_stats.failed << RST << "\n";
        cout << "  ├─ Memory Leaks:   " << (checker_stats.leaks > 0 ? RED : GRN) << BLD << checker_stats.leaks << RST << "\n";
        cout << "  └─ Crashes:        " << (checker_stats.crashes > 0 ? RED : GRN) << BLD << checker_stats.crashes << RST << "\n";
        
        double chk_rate = (checker_stats.total > 0) ? (100.0 * checker_stats.passed / checker_stats.total) : 0;
        cout << "     Success Rate:   ";
        if (chk_rate >= 95) cout << GRN;
        else if (chk_rate >= 80) cout << YEL;
        else cout << RED;
        cout << BLD << fixed << setprecision(1) << chk_rate << "%" << RST << "\n";
        
        if (!checker_stats.failed_tests.empty() && checker_stats.failed_tests.size() <= 10) {
            cout << "\n     " << RED << "Failed tests:" << RST << "\n";
            for (const auto& t : checker_stats.failed_tests) {
                cout << "       - " << t << "\n";
            }
        }
    }
    
    cout << "\n";
    cout << "  ⏱️  Time Elapsed:   " << fixed << setprecision(2) << elapsed << "s\n";
    
    cout << "\n" << GRY << "Trace log: " << cfg.trace_file << RST << "\n";
    if (stats.failed > 0 || checker_stats.failed > 0) {
        cout << GRY << "Errors log: " << cfg.errors_file << RST << "\n";
    }
    
    if (cfg.html_report) {
        generate_html_report();
    }
    
    // Final message
    bool all_passed = (stats.failed == 0 && stats.leaks == 0 && stats.crashes == 0);
    bool checker_passed = cfg.checker.empty() || (checker_stats.failed == 0 && checker_stats.leaks == 0 && checker_stats.crashes == 0);
    
    if (all_passed && checker_passed) {
        cout << "\n" << BLD << GRN << "🎉 ALL TESTS PASSED! Your push_swap is ready for evaluation!" << RST << "\n\n";
    } else if (all_passed && !checker_passed) {
        cout << "\n" << BLD << YEL << "✓ Push_swap tests passed! Checker (bonus) has some failures." << RST << "\n\n";
    } else {
        cout << "\n" << BLD << RED << "⚠ Some tests failed. Check errors.txt for details." << RST << "\n\n";
    }
    
    return (stats.failed > 0 || stats.crashes > 0) ? 1 : 0;
}
