#include "../include/VotingSystem.h"
#include <cstdint>

// ============================================================
//  SHA-256  (unchanged)
// ============================================================
static const uint32_t K256[64] = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,
    0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,
    0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,
    0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,
    0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,
    0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,
    0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,
    0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,
    0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};
static uint32_t rotr32(uint32_t x, unsigned n){ return (x>>n)|(x<<(32u-n)); }

std::string sha256(const std::string& input){
    uint32_t h0=0x6a09e667u,h1=0xbb67ae85u,h2=0x3c6ef372u,h3=0xa54ff53au;
    uint32_t h4=0x510e527fu,h5=0x9b05688cu,h6=0x1f83d9abu,h7=0x5be0cd19u;
    std::vector<uint8_t> msg(input.begin(),input.end());
    uint64_t bitLen=(uint64_t)msg.size()*8ULL;
    msg.push_back(0x80u);
    while(msg.size()%64!=56) msg.push_back(0x00u);
    for(int i=7;i>=0;--i) msg.push_back((uint8_t)((bitLen>>(i*8))&0xFFu));
    for(size_t chunk=0;chunk<msg.size();chunk+=64){
        uint32_t w[64];
        for(int i=0;i<16;++i)
            w[i]=((uint32_t)msg[chunk+i*4+0]<<24)|((uint32_t)msg[chunk+i*4+1]<<16)
                |((uint32_t)msg[chunk+i*4+2]<<8) |((uint32_t)msg[chunk+i*4+3]);
        for(int i=16;i<64;++i){
            uint32_t s0=rotr32(w[i-15],7)^rotr32(w[i-15],18)^(w[i-15]>>3);
            uint32_t s1=rotr32(w[i-2],17)^rotr32(w[i-2],19) ^(w[i-2]>>10);
            w[i]=w[i-16]+s0+w[i-7]+s1;
        }
        uint32_t a=h0,b=h1,c=h2,d=h3,e=h4,f=h5,g=h6,hh=h7;
        for(int i=0;i<64;++i){
            uint32_t S1=rotr32(e,6)^rotr32(e,11)^rotr32(e,25);
            uint32_t ch=(e&f)^(~e&g);
            uint32_t t1=hh+S1+ch+K256[i]+w[i];
            uint32_t S0=rotr32(a,2)^rotr32(a,13)^rotr32(a,22);
            uint32_t maj=(a&b)^(a&c)^(b&c);
            uint32_t t2=S0+maj;
            hh=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
        }
        h0+=a;h1+=b;h2+=c;h3+=d;h4+=e;h5+=f;h6+=g;h7+=hh;
    }
    std::ostringstream oss;
    oss<<std::hex<<std::setfill('0')
       <<std::setw(8)<<h0<<std::setw(8)<<h1<<std::setw(8)<<h2<<std::setw(8)<<h3
       <<std::setw(8)<<h4<<std::setw(8)<<h5<<std::setw(8)<<h6<<std::setw(8)<<h7;
    return oss.str();
}

// ============================================================
//  Console Helpers
// ============================================================
void setColor(int c)  { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),(WORD)c); }
void resetColor()     { setColor(CLR_RESET); }
void clearScreen()    { system("cls"); }

std::string getCurrentDateTime(){
    time_t now = time(nullptr);
    char buf[32];
    strftime(buf, sizeof(buf), "%d %b %Y   %H:%M:%S", localtime(&now));
    return std::string(buf);
}

// ============================================================
//  UI Constants
// ============================================================
static const int  UI_WIDTH = 72;
static const char H_LINE   = '-';
static const char B_LINE   = '=';

static std::string centerStr(const std::string& s, int width){
    int pad = (width - (int)s.size()) / 2;
    if (pad < 0) pad = 0;
    return std::string(pad, ' ') + s;
}

// ============================================================
//  pressAnyKey
// ============================================================
void pressAnyKey(){
    std::cout << "\n";
    setColor(CLR_CYAN);
    std::cout << "  " << std::string(UI_WIDTH - 2, H_LINE) << "\n";
    resetColor();
    setColor(CLR_YELLOW);
    std::cout << centerStr("Press ENTER to continue ...", UI_WIDTH) << "\n";
    resetColor();
    setColor(CLR_CYAN);
    std::cout << "  " << std::string(UI_WIDTH - 2, H_LINE) << "\n";
    resetColor();
    std::cin.ignore(10000, '\n');
}

// ============================================================
//  drawBox  (kept for compatibility)
// ============================================================
void drawBox(int width){
    setColor(CLR_CYAN);
    std::cout << "  +" << std::string(width - 4, '-') << "+\n";
    resetColor();
}

// ============================================================
//  printHeader
// ============================================================
void printHeader(const std::string& title){
    clearScreen();
    setColor(CLR_CYAN);
    std::cout << "\n  " << std::string(UI_WIDTH - 2, B_LINE) << "\n";
    setColor(CLR_GREEN);
    std::cout << centerStr("BANGLADESH ONLINE VOTING SYSTEM", UI_WIDTH) << "\n";
    setColor(CLR_CYAN);
    std::cout << "  " << std::string(UI_WIDTH - 2, H_LINE) << "\n";
    setColor(CLR_WHITE);
    std::cout << centerStr(title, UI_WIDTH) << "\n";
    setColor(CLR_CYAN);
    std::cout << "  " << std::string(UI_WIDTH - 2, B_LINE) << "\n";
    setColor(CLR_YELLOW);
    std::string dt = "  " + getCurrentDateTime();
    std::string powered = "Powered by C++ & MySQL  ";
    int gap = UI_WIDTH - (int)dt.size() - (int)powered.size();
    if (gap < 1) gap = 1;
    std::cout << dt << std::string(gap, ' ') << powered << "\n";
    setColor(CLR_CYAN);
    std::cout << "  " << std::string(UI_WIDTH - 2, H_LINE) << "\n\n";
    resetColor();
}

// ============================================================
//  Notification banners
// ============================================================
void printSuccess(const std::string& msg){
    std::cout << "\n";
    setColor(CLR_GREEN);
    std::cout << "  [ OK ]  " << msg << "\n";
    resetColor();
}
void printError(const std::string& msg){
    std::cout << "\n";
    setColor(CLR_RED);
    std::cout << "  [FAIL]  " << msg << "\n";
    resetColor();
}
void printWarning(const std::string& msg){
    std::cout << "\n";
    setColor(CLR_YELLOW);
    std::cout << "  [WARN]  " << msg << "\n";
    resetColor();
}
void printInfo(const std::string& msg){
    std::cout << "\n";
    setColor(CLR_CYAN);
    std::cout << "  [INFO]  " << msg << "\n";
    resetColor();
}

// ============================================================
//  Constituency list
// ============================================================
const std::vector<std::string> CONSTITUENCIES = {
    "Dhaka-1","Dhaka-2","Dhaka-3","Dhaka-4","Dhaka-5","Dhaka-6","Dhaka-7","Dhaka-8",
    "Dhaka-9","Dhaka-10","Dhaka-11","Dhaka-12","Dhaka-13","Dhaka-14","Dhaka-15",
    "Chattogram-1","Chattogram-2","Chattogram-3","Chattogram-4","Chattogram-5",
    "Chattogram-6","Chattogram-7","Chattogram-8","Chattogram-9","Chattogram-10",
    "Rajshahi-1","Rajshahi-2","Rajshahi-3","Rajshahi-4","Rajshahi-5","Rajshahi-6",
    "Khulna-1","Khulna-2","Khulna-3","Khulna-4","Khulna-5","Khulna-6",
    "Barishal-1","Barishal-2","Barishal-3","Barishal-4","Barishal-5","Barishal-6",
    "Sylhet-1","Sylhet-2","Sylhet-3","Sylhet-4","Sylhet-5","Sylhet-6",
    "Rangpur-1","Rangpur-2","Rangpur-3","Rangpur-4","Rangpur-5","Rangpur-6",
    "Mymensingh-1","Mymensingh-2","Mymensingh-3","Mymensingh-4","Mymensingh-5"
};

// ============================================================
//  Validation — Phone, NID
// ============================================================
bool isAllDigits(const std::string& s){
    if (s.empty()) return false;
    return std::all_of(s.begin(), s.end(), [](unsigned char c){ return std::isdigit(c); });
}

bool isValidNID(const std::string& nid){
    return isAllDigits(nid) && (nid.size()==10 || nid.size()==13 || nid.size()==17);
}

bool isValidPhone(const std::string& phone){
    // Must be exactly 11 digits, start with 0
    if (phone.size() != 11) return false;
    if (!isAllDigits(phone)) return false;
    if (phone[0] != '0') return false;
    return true;
}

// ============================================================
//  Validation — Name
//  Rules: 3-100 chars, letters/spaces/dots only, no all-spaces
// ============================================================
bool isValidName(const std::string& name){
    if (name.size() < 3 || name.size() > 100) return false;
    bool hasLetter = false;
    for (unsigned char c : name){
        if (std::isalpha(c))       { hasLetter = true; continue; }
        if (c == ' ' || c == '.') { continue; }
        return false;   // invalid character
    }
    return hasLetter;
}

// ============================================================
//  Validation — Address
//  Rules: 5-200 chars, not blank/whitespace-only
// ============================================================
bool isValidAddress(const std::string& addr){
    if (addr.size() < 5 || addr.size() > 200) return false;
    // must contain at least one non-space character
    return std::any_of(addr.begin(), addr.end(),
        [](unsigned char c){ return !std::isspace(c); });
}

// ============================================================
//  Helper: trim leading/trailing spaces from a string
// ============================================================
static std::string trim(const std::string& s){
    size_t st = s.find_first_not_of(' ');
    if (st == std::string::npos) return "";
    size_t en = s.find_last_not_of(' ');
    return s.substr(st, en - st + 1);
}

// ============================================================
//  Input readers
// ============================================================
std::string readValidatedNID(const std::string& label){
    std::string nid;
    while (true){
        setColor(CLR_CYAN);
        std::cout << "  " << std::left << std::setw(22) << label << ":  ";
        resetColor();
        std::getline(std::cin, nid);
        nid = trim(nid);
        if (isValidNID(nid)) return nid;
        printError("NID must be exactly 10, 13, or 17 digits (numbers only).");
    }
}

std::string readValidatedPhone(const std::string& label){
    std::string phone;
    while (true){
        setColor(CLR_CYAN);
        std::cout << "  " << std::left << std::setw(22) << label << ":  ";
        resetColor();
        std::getline(std::cin, phone);
        phone = trim(phone);
        if (isValidPhone(phone)) return phone;
        printError("Phone must be exactly 11 digits and start with 0  (e.g. 01XXXXXXXXX).");
    }
}

std::string readValidatedName(const std::string& label){
    std::string name;
    while (true){
        setColor(CLR_CYAN);
        std::cout << "  " << std::left << std::setw(22) << label << ":  ";
        resetColor();
        std::getline(std::cin, name);
        name = trim(name);
        if (isValidName(name)) return name;
        printError("Name must be 3-100 characters, letters and spaces only (no numbers/symbols).");
    }
}

std::string readValidatedAddress(const std::string& label){
    std::string address;
    while (true){
        setColor(CLR_CYAN);
        std::cout << "  " << std::left << std::setw(22) << label << ":  ";
        resetColor();
        std::getline(std::cin, address);
        address = trim(address);
        if (isValidAddress(address)) return address;
        printError("Address must be 5-200 characters and cannot be blank.");
    }
}

// ============================================================
//  Constituency selector — 4 per row, grouped
// ============================================================
std::string selectConstituency(){
    while (true){
        setColor(CLR_CYAN);
        std::cout << "\n  " << std::string(UI_WIDTH - 2, H_LINE) << "\n";
        std::cout << centerStr("SELECT CONSTITUENCY", UI_WIDTH) << "\n";
        std::cout << "  " << std::string(UI_WIDTH - 2, H_LINE) << "\n\n";
        resetColor();

        for (size_t i = 0; i < CONSTITUENCIES.size(); ++i){
            setColor(CLR_WHITE);
            std::cout << "  " << std::right << std::setw(3) << (i+1) << ". "
                      << std::left << std::setw(16) << CONSTITUENCIES[i];
            resetColor();
            if ((i + 1) % 4 == 0) std::cout << "\n";
        }
        std::cout << "\n\n";
        setColor(CLR_CYAN);
        std::cout << "  Enter number (1-" << CONSTITUENCIES.size() << ")  :  ";
        resetColor();

        int choice;
        if (!(std::cin >> choice)){ std::cin.clear(); std::cin.ignore(10000,'\n'); continue; }
        std::cin.ignore(10000, '\n');
        if (choice >= 1 && choice <= (int)CONSTITUENCIES.size())
            return CONSTITUENCIES[choice - 1];
        printError("Invalid selection. Enter a number between 1 and "
                   + std::to_string(CONSTITUENCIES.size()) + ".");
    }
}

// ============================================================
//  Date validation
// ============================================================
bool isValidDate(const std::string& dateStr){
    if (dateStr.size() != 10) return false;
    if (dateStr[4] != '-' || dateStr[7] != '-') return false;
    for (int i = 0; i < 10; ++i){
        if (i==4 || i==7) continue;
        if (!std::isdigit((unsigned char)dateStr[i])) return false;
    }
    int year  = std::stoi(dateStr.substr(0,4));
    int month = std::stoi(dateStr.substr(5,2));
    int day   = std::stoi(dateStr.substr(8,2));
    if (month < 1 || month > 12) return false;
    int dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    bool leap = (year%4==0 && (year%100!=0 || year%400==0));
    if (month==2 && leap) dim[1]=29;
    if (day < 1 || day > dim[month-1]) return false;
    time_t now = time(nullptr);
    tm* t = localtime(&now);
    int curYear = t->tm_year + 1900;
    if (year < 1900 || year > curYear) return false;
    return true;
}

std::string readValidatedDOB(const std::string& label, int minAge){
    std::string dob;
    while (true){
        setColor(CLR_CYAN);
        std::cout << "  " << std::left << std::setw(22) << label << ":  ";
        resetColor();
        std::getline(std::cin, dob);
        dob = trim(dob);
        if (!isValidDate(dob)){
            printError("Invalid date. Use format YYYY-MM-DD  (e.g. 1995-06-15).");
            continue;
        }
        int bY = std::stoi(dob.substr(0,4));
        int bM = std::stoi(dob.substr(5,2));
        int bD = std::stoi(dob.substr(8,2));
        time_t now = time(nullptr);
        tm* t = localtime(&now);
        int age = (t->tm_year+1900) - bY;
        if ((t->tm_mon+1) < bM || ((t->tm_mon+1)==bM && t->tm_mday < bD)) age--;
        if (age < minAge){
            printError("Must be at least " + std::to_string(minAge) + " years old to register.");
            continue;
        }
        if (age > 130){
            printError("Please enter a realistic date of birth.");
            continue;
        }
        return dob;
    }
}

// ============================================================
//  Masked password input
// ============================================================
std::string readPasswordMasked(const std::string& label){
    setColor(CLR_CYAN);
    std::cout << "  " << std::left << std::setw(22) << label << ":  ";
    resetColor();
    std::string password;
    char ch;
    while ((ch = _getch()) != '\r' && ch != '\n'){
        if (ch == '\b'){
            if (!password.empty()){
                password.pop_back();
                std::cout << "\b \b";
            }
        } else if (ch >= 32 && ch <= 126){
            password.push_back(ch);
            std::cout << '*';
        }
    }
    std::cout << "\n";
    return password;
}
