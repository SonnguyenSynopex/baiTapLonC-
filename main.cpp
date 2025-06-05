#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <random>
#include <algorithm>
#include <windows.h>
#include <unordered_map>
#include <fcntl.h>
#include <io.h>

// ===================== MÀU SẮC =====================
enum Color {
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    YELLOW = 6,
    WHITE = 7,
    GRAY = 8,
    LIGHT_BLUE = 9,
    LIGHT_GREEN = 10,
    LIGHT_CYAN = 11,
    LIGHT_RED = 12,
    LIGHT_MAGENTA = 13,
    LIGHT_YELLOW = 14,
    BRIGHT_WHITE = 15
};

void setColor(int textColor, int bgColor) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (bgColor << 4) | textColor);
}

void printColored(const std::string& text, int textColor = WHITE, int bgColor = BLACK) {
    setColor(textColor, bgColor);
    std::cout << text;
    setColor(WHITE, BLACK); // Reset color
}

void printHeader(const std::string& text) {
    std::cout << "\n";
    printColored("===== " + text + " =====\n", LIGHT_CYAN);
}

void printSuccess(const std::string& text) {
    printColored(text + "\n", LIGHT_GREEN);
}

void printError(const std::string& text) {
    printColored(text + "\n", LIGHT_RED);
}

void printWarning(const std::string& text) {
    printColored(text + "\n", LIGHT_YELLOW);
}

// ===================== CẤU TRÚC DỮ LIỆU =====================
struct User {
    std::string username;
    std::string password_hash;
    std::string full_name;
    std::string email;
    std::string role; // "user" hoặc "admin"
    std::string wallet_id;
    int balance = 0;
    bool auto_generated_password = false;
};

struct Transaction {
    std::string from_wallet;
    std::string to_wallet;
    int amount;
    std::string status; // "success" hoặc "fail"
    std::string time;
};

// ===================== HÀM TIỆN ÍCH =====================
std::string hash_password(const std::string& pw) {
    // Hash đơn giản, KHÔNG dùng cho thực tế!
    std::hash<std::string> hasher;
    return std::to_string(hasher(pw));
}

std::string now_str() {
    std::time_t t = std::time(nullptr);
    std::tm* tm_ptr = std::localtime(&t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_ptr);
    return buf;
}

std::string random_wallet_id() {
    static int counter = 0;
    std::stringstream ss;
    ss << "WALLET" << std::time(nullptr) << counter++;
    return ss.str();
}

std::string random_otp() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    return std::to_string(dis(gen));
}

void pause() {
    std::cout << "\nNhấn Enter để tiếp tục...";
    std::cin.ignore();
}

// ===================== LƯU/TRUY XUẤT USER =====================
std::string user_file(const std::string& username) {
    return "users/" + username + ".txt";
}

bool save_user(const User& u) {
    std::ofstream f(user_file(u.username));
    if (!f) return false;
    f << u.username << '\n' << u.password_hash << '\n' << u.full_name << '\n' << u.email << '\n'
      << u.role << '\n' << u.wallet_id << '\n' << u.balance << '\n' << u.auto_generated_password << '\n';
    return true;
}

bool load_user(const std::string& username, User& u) {
    std::ifstream f(user_file(username));
    if (!f) return false;
    std::getline(f, u.username);
    std::getline(f, u.password_hash);
    std::getline(f, u.full_name);
    std::getline(f, u.email);
    std::getline(f, u.role);
    std::getline(f, u.wallet_id);
    f >> u.balance;
    f >> u.auto_generated_password;
    return true;
}

bool user_exists(const std::string& username) {
    std::ifstream f(user_file(username));
    return f.good();
}

// ===================== OTP =====================
void save_otp(const std::string& username, const std::string& otp) {
    std::ofstream("otp_" + username + ".txt") << otp;
}

bool check_otp(const std::string& username, const std::string& otp) {
    std::ifstream f("otp_" + username + ".txt");
    std::string real_otp;
    std::getline(f, real_otp);
    return otp == real_otp;
}

void remove_otp(const std::string& username) {
    std::remove(("otp_" + username + ".txt").c_str());
}

// ===================== GIAO DỊCH =====================
void log_transaction(const Transaction& t) {
    std::ofstream f("transactions.log", std::ios::app);
    f << t.from_wallet << ',' << t.to_wallet << ',' << t.amount << ',' << t.status << ',' << t.time << '\n';
}

std::vector<Transaction> get_transactions(const std::string& wallet_id) {
    std::vector<Transaction> res;
    std::ifstream f("transactions.log");
    std::string line;
    while (std::getline(f, line)) {
        std::stringstream ss(line);
        Transaction t;
        std::getline(ss, t.from_wallet, ',');
        std::getline(ss, t.to_wallet, ',');
        std::string amount;
        std::getline(ss, amount, ',');
        t.amount = std::stoi(amount);
        std::getline(ss, t.status, ',');
        std::getline(ss, t.time);
        if (t.from_wallet == wallet_id || t.to_wallet == wallet_id)
            res.push_back(t);
    }
    return res;
}

// ===================== SAO LƯU =====================
void backup() {
    std::string backup_dir = "backup_" + now_str();
    std::replace(backup_dir.begin(), backup_dir.end(), ' ', '_');
    std::replace(backup_dir.begin(), backup_dir.end(), ':', '-');
    
    // Tạo thư mục backup
    CreateDirectoryA(backup_dir.c_str(), NULL);
    
    // Copy các file user
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA("users/*", &findFileData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string filename = findFileData.cFileName;
            if (filename != "." && filename != "..") {
                std::string source = "users/" + filename;
                std::string dest = backup_dir + "/" + filename;
                CopyFileA(source.c_str(), dest.c_str(), FALSE);
            }
        } while (FindNextFileA(hFind, &findFileData));
        FindClose(hFind);
    }
    
    // Copy file transactions.log
    CopyFileA("transactions.log", (backup_dir + "/transactions.log").c_str(), FALSE);
    
    std::cout << "Đã sao lưu vào thư mục: " << backup_dir << '\n';
}

// ===================== MENU & CHỨC NĂNG =====================
void register_user(const std::string& role = "user") {
    User u;
    std::cout << "\n--- Đăng ký tài khoản mới ---\n";
    std::cout << "Tên đăng nhập: ";
    std::getline(std::cin, u.username);
    if (user_exists(u.username)) {
        std::cout << "Tên đăng nhập đã tồn tại!\n";
        return;
    }
    std::cout << "Họ tên: "; std::getline(std::cin, u.full_name);
    std::cout << "Email: "; std::getline(std::cin, u.email);
    std::string pw;
    std::cout << "Nhập mật khẩu (bỏ trống để sinh tự động): ";
    std::getline(std::cin, pw);
    if (pw.empty()) {
        pw = random_otp();
        u.auto_generated_password = true;
        std::cout << "Mật khẩu tự sinh: " << pw << " (hãy đổi sau khi đăng nhập)\n";
    }
    u.password_hash = hash_password(pw);
    u.role = role;
    u.wallet_id = random_wallet_id();
    u.balance = 0;
    save_user(u);
    std::cout << "Đăng ký thành công!\n";
}

bool login(User& u) {
    std::cout << "\n--- Đăng nhập ---\n";
    std::string username, pw;
    std::cout << "Tên đăng nhập: "; std::getline(std::cin, username);
    std::cout << "Mật khẩu: "; std::getline(std::cin, pw);
    if (!load_user(username, u)) {
        std::cout << "Không tồn tại tài khoản!\n";
        return false;
    }
    if (u.password_hash != hash_password(pw)) {
        std::cout << "Sai mật khẩu!\n";
        return false;
    }
    if (u.auto_generated_password) {
        std::cout << "Bạn đang dùng mật khẩu tự sinh. Hãy đổi mật khẩu mới!\n";
        std::string newpw;
        std::cout << "Nhập mật khẩu mới: ";
        std::getline(std::cin, newpw);
        u.password_hash = hash_password(newpw);
        u.auto_generated_password = false;
        save_user(u);
        std::cout << "Đổi mật khẩu thành công!\n";
    }
    return true;
}

void change_info(User& u) {
    std::cout << "\n--- Đổi thông tin cá nhân ---\n";
    std::cout << "Họ tên mới: ";
    std::string newname; std::getline(std::cin, newname);
    std::cout << "Email mới: ";
    std::string newmail; std::getline(std::cin, newmail);
    std::string otp = random_otp();
    save_otp(u.username, otp);
    std::cout << "Mã OTP xác nhận (giả lập gửi email): " << otp << '\n';
    std::cout << "Nhập OTP: ";
    std::string user_otp; std::getline(std::cin, user_otp);
    if (check_otp(u.username, user_otp)) {
        u.full_name = newname;
        u.email = newmail;
        save_user(u);
        std::cout << "Cập nhật thành công!\n";
    } else {
        std::cout << "Sai OTP!\n";
    }
    remove_otp(u.username);
}

void change_password(User& u) {
    std::cout << "\n--- Đổi mật khẩu ---\n";
    std::string oldpw, newpw;
    std::cout << "Nhập mật khẩu cũ: "; std::getline(std::cin, oldpw);
    if (u.password_hash != hash_password(oldpw)) {
        std::cout << "Sai mật khẩu!\n";
        return;
    }
    std::cout << "Nhập mật khẩu mới: "; std::getline(std::cin, newpw);
    u.password_hash = hash_password(newpw);
    save_user(u);
    std::cout << "Đổi mật khẩu thành công!\n";
}

void view_balance(const User& u) {
    std::cout << "\nVí: " << u.wallet_id << "\nSố dư: " << u.balance << " điểm\n";
}

void view_transactions(const User& u) {
    std::cout << "\n--- Lịch sử giao dịch ---\n";
    auto txs = get_transactions(u.wallet_id);
    for (const auto& t : txs) {
        std::cout << t.time << ": " << t.from_wallet << " -> " << t.to_wallet << " | " << t.amount << " điểm | " << t.status << '\n';
    }
}

// Thêm hàm mới để hiển thị danh sách ví
void showWalletList(const std::string& exclude_wallet = "") {
    printHeader("DANH SÁCH VÍ");
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA("users/*", &findFileData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string filename = findFileData.cFileName;
            if (filename != "." && filename != "..") {
                std::string uname = filename.substr(0, filename.size() - 4);
                User tmp;
                if (load_user(uname, tmp) && tmp.wallet_id != exclude_wallet) {
                    printColored("Chủ ví: " + tmp.full_name + "\n", LIGHT_CYAN);
                    std::cout << "ID ví: " << tmp.wallet_id << "\n";
                    std::cout << "Số dư: " << tmp.balance << " điểm\n";
                    std::cout << "------------------------\n";
                }
            }
        } while (FindNextFileA(hFind, &findFileData));
        FindClose(hFind);
    }
}

void transfer(User& u) {
    printHeader("CHUYỂN ĐIỂM");
    std::cout << "Số dư hiện tại: " << u.balance << " điểm\n\n";
    
    // Hiển thị danh sách ví (trừ ví của người dùng hiện tại)
    showWalletList(u.wallet_id);
    
    std::string to_wallet;
    int amount;
    std::cout << "\nNhập ID ví nhận: ";
    std::getline(std::cin, to_wallet);
    std::cout << "Nhập số điểm muốn chuyển: ";
    std::cin >> amount;
    std::cin.ignore();
    
    User target;
    bool found = false;
    
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA("users/*", &findFileData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string filename = findFileData.cFileName;
            if (filename != "." && filename != "..") {
                std::string uname = filename.substr(0, filename.size() - 4);
                User tmp;
                if (load_user(uname, tmp) && tmp.wallet_id == to_wallet) {
                    target = tmp;
                    found = true;
                    break;
                }
            }
        } while (FindNextFileA(hFind, &findFileData));
        FindClose(hFind);
    }
    
    if (!found) {
        printError("Không tìm thấy ví nhận!");
        return;
    }
    
    if (u.balance < amount) {
        printError("Số dư không đủ!");
        return;
    }

    // OTP xác nhận
    std::string otp = random_otp();
    save_otp(u.username, otp);
    std::cout << "Mã OTP xác nhận chuyển điểm (giả lập gửi email): " << otp << '\n';
    std::cout << "Nhập OTP: ";
    std::string user_otp;
    std::getline(std::cin, user_otp);
    
    if (!check_otp(u.username, user_otp)) {
        printError("Sai OTP!");
        remove_otp(u.username);
        return;
    }
    remove_otp(u.username);
    
    // Giao dịch atomic
    u.balance -= amount;
    target.balance += amount;
    save_user(u);
    save_user(target);
    
    Transaction t{u.wallet_id, target.wallet_id, amount, "success", now_str()};
    log_transaction(t);
    
    printSuccess("Chuyển điểm thành công!");
    std::cout << "Số dư mới: " << u.balance << " điểm\n";
}

// ===================== TÍNH NĂNG MỚI ==========================
void recoverPassword() {
    std::string username, email;
    printHeader("KHÔI PHỤC MẬT KHẨU");
    std::cout << "Nhập tên đăng nhập: ";
    std::getline(std::cin, username);
    std::cout << "Nhập email đăng ký: ";
    std::getline(std::cin, email);

    User u;
    if (!load_user(username, u) || u.email != email) {
        printError("Thông tin không chính xác!");
        return;
    }

    std::string newPassword = random_otp();
    u.password_hash = hash_password(newPassword);
    u.auto_generated_password = true;
    save_user(u);

    printSuccess("Mật khẩu mới của bạn là: " + newPassword);
    printWarning("Vui lòng đổi mật khẩu sau khi đăng nhập!");
}

void deleteAccount(User& currentUser) {
    printHeader("XÓA TÀI KHOẢN");
    printWarning("Cảnh báo: Hành động này không thể hoàn tác!");
    std::cout << "Nhập mật khẩu để xác nhận: ";
    std::string password;
    std::getline(std::cin, password);

    if (currentUser.password_hash != hash_password(password)) {
        printError("Sai mật khẩu!");
        return;
    }

    // Xác nhận bằng OTP
    std::string otp = random_otp();
    save_otp(currentUser.username, otp);
    std::cout << "Mã OTP xác nhận (giả lập gửi email): " << otp << '\n';
    std::cout << "Nhập OTP: ";
    std::string user_otp;
    std::getline(std::cin, user_otp);

    if (!check_otp(currentUser.username, user_otp)) {
        printError("Sai OTP!");
        remove_otp(currentUser.username);
        return;
    }
    remove_otp(currentUser.username);

    // Xóa file user
    std::remove(user_file(currentUser.username).c_str());
    printSuccess("Tài khoản đã được xóa thành công!");
}

void addPoints(User& user) {
    printHeader("NẠP ĐIỂM VÀO VÍ");
    int amount;
    std::cout << "Nhập số điểm muốn nạp: ";
    std::cin >> amount;
    std::cin.ignore();

    if (amount <= 0) {
        printError("Số điểm phải lớn hơn 0!");
        return;
    }

    // Xác nhận bằng OTP
    std::string otp = random_otp();
    save_otp(user.username, otp);
    std::cout << "Mã OTP xác nhận (giả lập gửi email): " << otp << '\n';
    std::cout << "Nhập OTP: ";
    std::string user_otp;
    std::getline(std::cin, user_otp);

    if (!check_otp(user.username, user_otp)) {
        printError("Sai OTP!");
        remove_otp(user.username);
        return;
    }
    remove_otp(user.username);

    user.balance += amount;
    save_user(user);

    // Ghi log giao dịch
    Transaction t{"SYSTEM", user.wallet_id, amount, "success", now_str()};
    log_transaction(t);

    printSuccess("Nạp điểm thành công!");
    std::cout << "Số dư mới: " << user.balance << " điểm\n";
}

void viewAccountInfo(const User& u) {
    printHeader("THÔNG TIN TÀI KHOẢN");
    printColored("Thông tin cá nhân:\n", LIGHT_CYAN);
    std::cout << "Tên đăng nhập: " << u.username << "\n";
    std::cout << "Họ và tên: " << u.full_name << "\n";
    std::cout << "Email: " << u.email << "\n";
    std::cout << "Vai trò: " << (u.role == "admin" ? "Quản trị viên" : "Người dùng") << "\n\n";
    
    printColored("Thông tin ví:\n", LIGHT_CYAN);
    std::cout << "ID ví: " << u.wallet_id << "\n";
    std::cout << "Số dư: " << u.balance << " điểm\n";
    
    // Hiển thị thống kê giao dịch
    auto txs = get_transactions(u.wallet_id);
    int total_sent = 0, total_received = 0;
    for (const auto& t : txs) {
        if (t.from_wallet == u.wallet_id) {
            total_sent += t.amount;
        } else {
            total_received += t.amount;
        }
    }
    
    printColored("\nThống kê giao dịch:\n", LIGHT_CYAN);
    std::cout << "Tổng số giao dịch: " << txs.size() << "\n";
    std::cout << "Tổng điểm đã chuyển: " << total_sent << "\n";
    std::cout << "Tổng điểm đã nhận: " << total_received << "\n";
}

void user_menu(User& u) {
    while (true) {
        printHeader("MENU NGƯỜI DÙNG");
        std::cout << "1. Xem thông tin tài khoản\n";
        std::cout << "2. Xem số dư\n";
        std::cout << "3. Xem lịch sử giao dịch\n";
        std::cout << "4. Chuyển điểm\n";
        std::cout << "5. Nạp điểm\n";
        std::cout << "6. Đổi thông tin cá nhân\n";
        std::cout << "7. Đổi mật khẩu\n";
        std::cout << "8. Xóa tài khoản\n";
        std::cout << "9. Thoát\n";
        printColored("Chọn: ", LIGHT_CYAN);
        
        int c; std::cin >> c; std::cin.ignore();
        if (c == 1) viewAccountInfo(u);
        else if (c == 2) view_balance(u);
        else if (c == 3) view_transactions(u);
        else if (c == 4) transfer(u);
        else if (c == 5) addPoints(u);
        else if (c == 6) change_info(u);
        else if (c == 7) change_password(u);
        else if (c == 8) {
            deleteAccount(u);
            return; // Thoát sau khi xóa tài khoản
        }
        else if (c == 9) break;
        else printError("Sai lựa chọn!");
        pause();
    }
}

void admin_menu(User& u) {
    while (true) {
        std::cout << "\n===== MENU QUẢN LÝ =====\n";
        std::cout << "1. Tạo tài khoản mới\n2. Xem danh sách tài khoản\n3. Sao lưu dữ liệu\n4. Thoát\nChọn: ";
        int c; std::cin >> c; std::cin.ignore();
        if (c == 1) register_user();
        else if (c == 2) {
            std::cout << "\n--- Danh sách tài khoản ---\n";
            WIN32_FIND_DATAA findFileData;
            HANDLE hFind = FindFirstFileA("users/*", &findFileData);
            
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    std::string filename = findFileData.cFileName;
                    if (filename != "." && filename != "..") {
                        std::string uname = filename.substr(0, filename.size() - 4);
                        User tmp;
                        if (load_user(uname, tmp)) {
                            std::cout << "Tên: " << tmp.username << ", Họ tên: " << tmp.full_name 
                                     << ", Email: " << tmp.email << ", Ví: " << tmp.wallet_id 
                                     << ", Số dư: " << tmp.balance << '\n';
                        }
                    }
                } while (FindNextFileA(hFind, &findFileData));
                FindClose(hFind);
            }
        }
        else if (c == 3) backup();
        else if (c == 4) break;
        else std::cout << "Sai lựa chọn!\n";
        pause();
    }
}

void ensure_dirs() {
    CreateDirectoryA("users", NULL);
    std::ofstream("transactions.log", std::ios::app); // tạo file nếu chưa có
}

void create_admin() {
    if (!user_exists("admin")) {
        User u;
        u.username = "admin";
        u.full_name = "Quản trị viên";
        u.email = "admin@system.com";
        u.role = "admin";
        u.wallet_id = random_wallet_id();
        u.balance = 0;
        u.password_hash = hash_password("admin123");
        u.auto_generated_password = false;
        save_user(u);
    }
}

int main() {
    // Cấu hình console để hiển thị Unicode tiếng Việt
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    ensure_dirs();
    create_admin();
    while (true) {
        printHeader("HỆ THỐNG QUẢN LÝ VÍ ĐIỂM THƯỞNG");
        std::cout << "1. Đăng nhập\n";
        std::cout << "2. Đăng ký\n";
        std::cout << "3. Khôi phục mật khẩu\n";
        std::cout << "4. Thoát\n";
        printColored("Chọn: ", LIGHT_CYAN);
        
        int c; std::cin >> c; std::cin.ignore();
        if (c == 1) {
            User u;
            if (login(u)) {
                if (u.role == "admin") admin_menu(u);
                else user_menu(u);
            }
        } else if (c == 2) register_user();
        else if (c == 3) recoverPassword();
        else if (c == 4) break;
        else printError("Sai lựa chọn!");
    }
    printSuccess("Tạm biệt!");
    return 0;
} 