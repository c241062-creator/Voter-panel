#include "../include/VotingSystem.h"

// ============================================================
//  VoterPanel
// ============================================================
VoterPanel::VoterPanel(DatabaseManager& db, Voter& voter) : db(db), voter(voter) {}

void VoterPanel::run() {
    while (true) {
        printHeader("VOTER PORTAL");
        setColor(CLR_GREEN);
        std::cout << "  Welcome, " << voter.fullName << "\n";
        setColor(CLR_WHITE);
        std::cout << "  NID          : " << voter.nidNumber << "\n";
        std::cout << "  Constituency : " << voter.constituency << "\n\n";
        resetColor();

        setColor(CLR_WHITE);
        std::cout << "  [1]  Cast My Vote\n";
        std::cout << "  [2]  Check My Voting Status\n";
        std::cout << "  [0]  Logout\n\n";
        resetColor();

        setColor(CLR_CYAN); std::cout << "  Choice: "; resetColor();
        int c; std::cin >> c; std::cin.ignore(10000,'\n');

        switch(c){
            case 1: castVote();     break;
            case 2: viewMyStatus(); break;
            case 0: return;
            default: printError("Invalid option."); pressAnyKey();
        }
    }
}

void VoterPanel::castVote() {
    printHeader("CAST YOUR VOTE");

    auto elections = db.getAllElections();
    std::vector<Election> active;
    for (auto& e : elections)
        if (e.status == "ACTIVE") active.push_back(e);

    if (active.empty()) {
        printWarning("No active elections at the moment.");
        pressAnyKey(); return;
    }

    setColor(CLR_CYAN);
    std::cout << "  Active Elections:\n";
    std::cout << "  " << std::string(55,'-') << "\n";
    resetColor();
    for (auto& e : active) {
        setColor(CLR_WHITE);
        std::cout << "  [" << e.id << "] " << e.title << " (" << e.constituency << ")\n";
        resetColor();
    }

    setColor(CLR_CYAN); std::cout << "\n  Select Election ID: "; resetColor();
    int eid; std::cin >> eid; std::cin.ignore(10000,'\n');

    if (db.hasVoted(voter.id, eid)) {
        printWarning("You have ALREADY voted in this election.");
        db.logEvent("DOUBLE_VOTE_ATTEMPT","VOTER",voter.id,
            "Voter tried to vote again in election #"+std::to_string(eid));
        pressAnyKey(); return;
    }

    auto allCandidates = db.getCandidatesByElection(eid);
    std::vector<Candidate> candidates;
    for (auto& c : allCandidates)
        if (c.constituency == voter.constituency) candidates.push_back(c);

    if (candidates.empty()) {
        printWarning("No candidates registered for your constituency ("
                     + voter.constituency + ") in this election.");
        pressAnyKey(); return;
    }

    std::cout << "\n";
    setColor(CLR_CYAN);
    std::cout << "  " << std::left
              << std::setw(6)  << "ID"
              << std::setw(28) << "Candidate Name"
              << std::setw(28) << "Party"
              << "Symbol\n";
    std::cout << "  " << std::string(70,'-') << "\n";
    resetColor();

    for (auto& c : candidates) {
        setColor(CLR_WHITE);
        std::cout << "  " << std::left
                  << std::setw(6)  << c.id
                  << std::setw(28) << c.fullName.substr(0,26)
                  << std::setw(28) << c.partyName.substr(0,26)
                  << c.partySymbol << "\n";
        resetColor();
    }

    setColor(CLR_CYAN); std::cout << "\n  Enter Candidate ID to vote for: "; resetColor();
    int cid; std::cin >> cid; std::cin.ignore(10000,'\n');

    std::string selectedName;
    for (auto& c : candidates)
        if (c.id == cid) selectedName = c.fullName + " (" + c.partyName + ")";

    if (selectedName.empty()) { printError("Invalid candidate ID."); pressAnyKey(); return; }

    setColor(CLR_YELLOW);
    std::cout << "\n  You selected : " << selectedName << "\n";
    std::cout << "  Confirm vote? (y/n): ";
    resetColor();
    std::string ans; std::getline(std::cin, ans);

    if (ans != "y" && ans != "Y") {
        printInfo("Vote cancelled."); pressAnyKey(); return;
    }

    if (db.castVote(voter.id, eid, cid, "127.0.0.1")) {
        printSuccess("VOTE CAST SUCCESSFULLY!");
        setColor(CLR_GREEN);
        std::cout << "  Voted for: " << selectedName << "\n";
        std::cout << "  Thank you for participating in democracy!\n";
        resetColor();
        db.logEvent("VOTE_CAST","VOTER",voter.id,
            "Cast vote for candidate #"+std::to_string(cid)
            +" in election #"+std::to_string(eid));
    } else {
        printError("Vote failed. You may have already voted.");
    }
    pressAnyKey();
}

void VoterPanel::viewMyStatus() {
    printHeader("MY VOTING STATUS");
    auto elections = db.getAllElections();
    if (elections.empty()) { printWarning("No elections found."); pressAnyKey(); return; }

    setColor(CLR_CYAN);
    std::cout << "  " << std::left
              << std::setw(6)  << "ID"
              << std::setw(35) << "Election"
              << std::setw(12) << "Status"
              << "My Vote\n";
    std::cout << "  " << std::string(70,'-') << "\n";
    resetColor();

    for (auto& e : elections) {
        bool voted = db.hasVoted(voter.id, e.id);
        if (voted) setColor(CLR_GREEN); else setColor(CLR_WHITE);
        std::cout << "  " << std::left
                  << std::setw(6)  << e.id
                  << std::setw(35) << e.title.substr(0,33)
                  << std::setw(12) << e.status
                  << (voted ? "VOTED" : "NOT VOTED") << "\n";
        resetColor();
    }
    pressAnyKey();
}

// ============================================================
//  LoginScreen
// ============================================================
LoginScreen::LoginScreen(DatabaseManager& db) : db(db) {}

void LoginScreen::run() {
    while (true) {
        printHeader("BANGLADESH ONLINE VOTING SYSTEM");
        setColor(CLR_YELLOW);
        std::cout << "      *** Welcome to the Bangladesh Digital Election Portal ***\n\n";
        resetColor();
        setColor(CLR_WHITE);
        std::cout << "  [1]  Admin Login\n";
        std::cout << "  [2]  Voter Login\n";
        std::cout << "  [3]  Register as Voter\n";
        std::cout << "  [0]  Exit\n\n";
        resetColor();
        setColor(CLR_CYAN); std::cout << "  Select option: "; resetColor();
        int c; std::cin >> c; std::cin.ignore(10000,'\n');
        switch(c){
            case 1: adminLogin();    break;
            case 2: voterLogin();    break;
            case 3: voterRegister(); break;
            case 0:
                setColor(CLR_YELLOW);
                std::cout << "\n  Thank you for using Bangladesh Online Voting System.\n\n";
                resetColor();
                return;
            default: printError("Invalid choice."); pressAnyKey();
        }
    }
}

void LoginScreen::adminLogin() {
    printHeader("ADMIN LOGIN");
    std::string username, password;
    setColor(CLR_CYAN); std::cout << "  " << std::left << std::setw(22) << "Username" << ":  "; resetColor();
    std::getline(std::cin, username);
    password = readPasswordMasked("Password");

    if (db.adminLogin(username, password)) {
        printSuccess("Admin login successful!");
        db.logEvent("ADMIN_LOGIN","ADMIN",1,"Admin logged in: "+username);
        pressAnyKey();
        AdminPanel panel(db, 1);
        panel.run();
    } else {
        printError("Invalid username or password.");
        db.logEvent("ADMIN_LOGIN_FAILED","ADMIN",0,"Failed login for: "+username);
        pressAnyKey();
    }
}

void LoginScreen::voterLogin() {
    printHeader("VOTER LOGIN");
    std::string nid, password;
    setColor(CLR_CYAN); std::cout << "  " << std::left << std::setw(22) << "NID Number" << ":  "; resetColor();
    std::getline(std::cin, nid);

    if (db.getFailedLoginCount(nid, 10) >= 5) {
        printError("Account locked. Too many failed attempts. Try after 10 minutes.");
        pressAnyKey(); return;
    }

    password = readPasswordMasked("Password");

    Voter voter;
    if (db.voterLogin(nid, password, voter)) {
        if (!voter.isActive) {
            printError("Account deactivated. Contact admin.");
            pressAnyKey(); return;
        }
        db.recordLoginAttempt(nid, true);
        printSuccess("Login successful! Welcome, " + voter.fullName);
        pressAnyKey();
        VoterPanel panel(db, voter);
        panel.run();
    } else {
        db.recordLoginAttempt(nid, false);
        printError("Invalid NID or password.");
        int fails = db.getFailedLoginCount(nid, 10);
        if (fails >= 3) {
            setColor(CLR_RED);
            std::cout << "  WARNING: " << fails << " failed attempts. Locks at 5.\n";
            resetColor();
        }
        pressAnyKey();
    }
}

void LoginScreen::voterRegister() {
    printHeader("VOTER SELF-REGISTRATION");
    Voter v; std::string password, confirmPass;

    setColor(CLR_YELLOW);
    std::cout << "  Fill in your details below.\n\n";
    resetColor();

    // --- validated inputs ---
    v.fullName    = readValidatedName("Full Name");
    v.nidNumber   = readValidatedNID("NID Number");

    if (db.voterExists(v.nidNumber)) {
        printError("This NID is already registered.");
        pressAnyKey(); return;
    }

    v.dob          = readValidatedDOB("Date of Birth", 18);
    v.constituency = selectConstituency();
    v.address      = readValidatedAddress("Address");
    v.phone        = readValidatedPhone("Phone Number");
    password       = readPasswordMasked("Set Password");
    confirmPass    = readPasswordMasked("Confirm Password");

    if (password != confirmPass) { printError("Passwords do not match!"); pressAnyKey(); return; }
    if (password.length() < 6)  { printError("Password must be at least 6 characters!"); pressAnyKey(); return; }

    if (db.registerVoter(v, password)) {
        printSuccess("Registration successful! You can now login with your NID.");
    } else {
        printError("Registration failed. Try again.");
    }
    pressAnyKey();
}
