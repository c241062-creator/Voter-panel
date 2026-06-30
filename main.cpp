#include "../include/VotingSystem.h"
#include <fstream>

struct Config { std::string dsn="VotingSystemDSN",user="root",pass=""; };

Config loadConfig(const std::string& path) {
    Config cfg;
    std::ifstream f(path);
    if (!f.is_open()) return cfg;
    std::string line;
    while (std::getline(f,line)) {
        if (line.empty()||line[0]=='#') continue;
        auto sep=line.find('=');
        if (sep==std::string::npos) continue;
        std::string key=line.substr(0,sep), val=line.substr(sep+1);
        if      (key=="DSN")  cfg.dsn=val;
        else if (key=="USER") cfg.user=val;
        else if (key=="PASS") cfg.pass=val;
    }
    return cfg;
}

void showSplash() {
    clearScreen();
    setColor(CLR_GREEN);
    std::cout << "\n";
    std::cout << "  ================================================================\n";
    std::cout << "  |                                                              |\n";
    std::cout << "  |       BANGLADESH ONLINE VOTING SYSTEM  v1.0                 |\n";
    std::cout << "  |       Powered by C++ & MySQL                                |\n";
    std::cout << "  |                                                              |\n";
    std::cout << "  |       Ensuring Fair, Transparent and Secure Elections        |\n";
    std::cout << "  |                                                              |\n";
    std::cout << "  ================================================================\n\n";
    setColor(CLR_YELLOW);
    std::cout << "                    Loading system...\n\n";
    resetColor();
    Sleep(1200);
}

int main() {
    SetConsoleTitle("Bangladesh Online Voting System");
    showSplash();

    Config cfg = loadConfig("db.cfg");

    setColor(CLR_CYAN);
    std::cout << "  Connecting to database (DSN: " << cfg.dsn << ")...\n";
    resetColor();

    DatabaseManager db;
    if (!db.connect(cfg.dsn, cfg.user, cfg.pass)) {
        setColor(CLR_RED);
        std::cout << "\n  FATAL: Cannot connect to database!\n";
        std::cout << "  Check:\n";
        std::cout << "    1. MySQL service is running\n";
        std::cout << "    2. ODBC DSN '" << cfg.dsn << "' is configured\n";
        std::cout << "    3. Credentials in db.cfg are correct\n";
        std::cout << "    4. Database 'bangladesh_voting_system' exists\n\n";
        resetColor();
        pressAnyKey();
        return 1;
    }

    setColor(CLR_GREEN);
    std::cout << "  Database connected successfully!\n";
    resetColor();
    Sleep(800);

    LoginScreen loginScreen(db);
    loginScreen.run();

    db.disconnect();
    return 0;
}
