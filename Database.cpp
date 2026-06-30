// ============================================================
//   BANGLADESH ONLINE VOTING SYSTEM
//   DatabaseManager Implementation
// ============================================================
#include "../include/VotingSystem.h"
#include <stdexcept>
#include <cstring>

// ============================================================
//  Constructor / Destructor
// ============================================================
DatabaseManager::DatabaseManager() {
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
}

DatabaseManager::~DatabaseManager() {
    disconnect();
    if (hDbc != SQL_NULL_HDBC)  SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    if (hEnv != SQL_NULL_HENV)  SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}

// ============================================================
//  Connect / Disconnect
// ============================================================
bool DatabaseManager::connect(const std::string& dsn,
                              const std::string& user,
                              const std::string& pass) {
    SQLRETURN ret = SQLConnect(hDbc,
        (SQLCHAR*)dsn.c_str(),  SQL_NTS,
        (SQLCHAR*)user.c_str(), SQL_NTS,
        (SQLCHAR*)pass.c_str(), SQL_NTS);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        connected = true;
        return true;
    }
    checkError(hDbc, SQL_HANDLE_DBC, "connect");
    return false;
}

void DatabaseManager::disconnect() {
    if (connected) {
        SQLDisconnect(hDbc);
        connected = false;
    }
}

bool DatabaseManager::checkError(SQLHANDLE handle, SQLSMALLINT type, const std::string& ctx) {
    SQLCHAR state[6], msg[512];
    SQLINTEGER nativeErr;
    SQLSMALLINT msgLen;
    SQLGetDiagRec(type, handle, 1, state, &nativeErr, msg, sizeof(msg), &msgLen);
    printError(ctx + " => " + std::string((char*)msg));
    return false;
}

// ============================================================
//  executeNonQuery  (INSERT / UPDATE / DELETE / DDL)
// ============================================================
bool DatabaseManager::executeNonQuery(const std::string& sql) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    SQLRETURN ret = SQLExecDirect(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    bool ok = (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
    if (!ok) checkError(hStmt, SQL_HANDLE_STMT, "executeNonQuery");
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return ok;
}

// ============================================================
//  Admin Login
// ============================================================
bool DatabaseManager::adminLogin(const std::string& username, const std::string& password) {
    std::string hash = sha256(password);
    std::string sql  = "SELECT admin_id FROM admins WHERE username='" + username +
                       "' AND password_hash='" + hash + "'";
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    SQLRETURN ret = SQLExecDirect(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    bool found = false;
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
        found = (SQLFetch(hStmt) == SQL_SUCCESS);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return found;
}

// ============================================================
//  Elections
// ============================================================
bool DatabaseManager::addElection(const std::string& title, const std::string& desc,
                                  const std::string& constituency,
                                  const std::string& startDate, const std::string& endDate,
                                  int adminId) {
    std::string sql = "INSERT INTO elections (title,description,constituency,start_date,end_date,status,created_by) "
                      "VALUES ('" + title + "','" + desc + "','" + constituency + "','" +
                      startDate + "','" + endDate + "','UPCOMING'," + std::to_string(adminId) + ")";
    return executeNonQuery(sql);
}

std::vector<Election> DatabaseManager::getAllElections() {
    std::vector<Election> list;
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    std::string sql = "SELECT election_id,title,description,constituency,start_date,end_date,status FROM elections ORDER BY election_id DESC";
    if (SQLExecDirect(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS) != SQL_SUCCESS &&
        SQLExecDirect(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS) != SQL_SUCCESS_WITH_INFO) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return list;
    }
    char id[16],title[256],desc[512],con[128],sd[32],ed[32],status[16];
    SQLLEN len;
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        Election e;
        SQLGetData(hStmt,1,SQL_C_CHAR,id,   sizeof(id),   &len); e.id           = atoi(id);
        SQLGetData(hStmt,2,SQL_C_CHAR,title, sizeof(title),&len); e.title        = title;
        SQLGetData(hStmt,3,SQL_C_CHAR,desc,  sizeof(desc), &len); e.description  = (len==SQL_NULL_DATA)?"":desc;
        SQLGetData(hStmt,4,SQL_C_CHAR,con,   sizeof(con),  &len); e.constituency = con;
        SQLGetData(hStmt,5,SQL_C_CHAR,sd,    sizeof(sd),   &len); e.startDate    = sd;
        SQLGetData(hStmt,6,SQL_C_CHAR,ed,    sizeof(ed),   &len); e.endDate      = ed;
        SQLGetData(hStmt,7,SQL_C_CHAR,status,sizeof(status),&len);e.status       = status;
        list.push_back(e);
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return list;
}

bool DatabaseManager::updateElectionStatus(int electionId, const std::string& status) {
    std::string sql = "UPDATE elections SET status='" + status +
                      "' WHERE election_id=" + std::to_string(electionId);
    return executeNonQuery(sql);
}

bool DatabaseManager::deleteElection(int electionId) {
    // Only allow deleting UPCOMING elections with no votes
    std::string sql = "DELETE FROM elections WHERE election_id=" + std::to_string(electionId) +
                      " AND status='UPCOMING' AND election_id NOT IN (SELECT election_id FROM votes)";
    return executeNonQuery(sql);
}

// ============================================================
//  Candidates
// ============================================================
bool DatabaseManager::addCandidate(const Candidate& c) {
    std::string sql = "INSERT INTO candidates (election_id,full_name,nid_number,party_name,"
                      "party_symbol,constituency,date_of_birth,address,phone) VALUES (" +
                      std::to_string(c.electionId) + ",'" + c.fullName + "','" + c.nidNumber + "','" +
                      c.partyName + "','" + c.partySymbol + "','" + c.constituency + "','" +
                      c.dob + "','" + c.address + "','" + c.phone + "')";
    return executeNonQuery(sql);
}

std::vector<Candidate> DatabaseManager::getCandidatesByElection(int electionId) {
    std::vector<Candidate> list;
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    std::string sql = "SELECT candidate_id,election_id,full_name,nid_number,party_name,"
                      "party_symbol,constituency,date_of_birth,address,phone,is_active "
                      "FROM candidates WHERE election_id=" + std::to_string(electionId) +
                      " AND is_active=1 ORDER BY full_name";
    if (SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)!=SQL_SUCCESS &&
        SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)!=SQL_SUCCESS_WITH_INFO) {
        SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return list;
    }
    char cid[16],eid[16],fn[128],nid[32],party[128],sym[64],con[128],dob[32],addr[256],ph[32],active[8];
    SQLLEN len;
    while (SQLFetch(hStmt)==SQL_SUCCESS) {
        Candidate c;
        SQLGetData(hStmt,1,SQL_C_CHAR,cid,  sizeof(cid),  &len); c.id           = atoi(cid);
        SQLGetData(hStmt,2,SQL_C_CHAR,eid,  sizeof(eid),  &len); c.electionId   = atoi(eid);
        SQLGetData(hStmt,3,SQL_C_CHAR,fn,   sizeof(fn),   &len); c.fullName     = fn;
        SQLGetData(hStmt,4,SQL_C_CHAR,nid,  sizeof(nid),  &len); c.nidNumber    = nid;
        SQLGetData(hStmt,5,SQL_C_CHAR,party,sizeof(party),&len); c.partyName    = party;
        SQLGetData(hStmt,6,SQL_C_CHAR,sym,  sizeof(sym),  &len); c.partySymbol  = (len==SQL_NULL_DATA)?"":sym;
        SQLGetData(hStmt,7,SQL_C_CHAR,con,  sizeof(con),  &len); c.constituency = con;
        SQLGetData(hStmt,8,SQL_C_CHAR,dob,  sizeof(dob),  &len); c.dob          = (len==SQL_NULL_DATA)?"":dob;
        SQLGetData(hStmt,9,SQL_C_CHAR,addr, sizeof(addr), &len); c.address      = (len==SQL_NULL_DATA)?"":addr;
        SQLGetData(hStmt,10,SQL_C_CHAR,ph,  sizeof(ph),   &len); c.phone        = (len==SQL_NULL_DATA)?"":ph;
        SQLGetData(hStmt,11,SQL_C_CHAR,active,sizeof(active),&len); c.isActive  = atoi(active);
        list.push_back(c);
    }
    SQLFreeHandle(SQL_HANDLE_STMT,hStmt);
    return list;
}

bool DatabaseManager::removeCandidate(int candidateId) {
    std::string sql = "UPDATE candidates SET is_active=0 WHERE candidate_id=" + std::to_string(candidateId);
    return executeNonQuery(sql);
}

bool DatabaseManager::candidateExists(const std::string& nid) {
    SQLHSTMT hStmt; SQLAllocHandle(SQL_HANDLE_STMT,hDbc,&hStmt);
    std::string sql = "SELECT COUNT(*) FROM candidates WHERE nid_number='" + nid + "'";
    bool exists = false;
    if (SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)==SQL_SUCCESS ||
        SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)==SQL_SUCCESS_WITH_INFO) {
        char cnt[16]; SQLLEN len;
        if (SQLFetch(hStmt)==SQL_SUCCESS) {
            SQLGetData(hStmt,1,SQL_C_CHAR,cnt,sizeof(cnt),&len);
            exists = atoi(cnt) > 0;
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return exists;
}

// ============================================================
//  Voters
// ============================================================
bool DatabaseManager::registerVoter(const Voter& v, const std::string& password) {
    std::string hash = sha256(password);
    std::string sql = "INSERT INTO voters (full_name,nid_number,date_of_birth,constituency,"
                      "address,phone,password_hash) VALUES ('" +
                      v.fullName + "','" + v.nidNumber + "','" + v.dob + "','" +
                      v.constituency + "','" + v.address + "','" + v.phone + "','" + hash + "')";
    return executeNonQuery(sql);
}

bool DatabaseManager::voterLogin(const std::string& nid, const std::string& password, Voter& out) {
    std::string hash = sha256(password);
    SQLHSTMT hStmt; SQLAllocHandle(SQL_HANDLE_STMT,hDbc,&hStmt);
    std::string sql = "SELECT voter_id,full_name,nid_number,date_of_birth,constituency,"
                      "address,phone,is_active FROM voters WHERE nid_number='" + nid +
                      "' AND password_hash='" + hash + "'";
    bool found = false;
    if (SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)==SQL_SUCCESS ||
        SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)==SQL_SUCCESS_WITH_INFO) {
        if (SQLFetch(hStmt)==SQL_SUCCESS) {
            char id[16],fn[128],nidv[32],dob[32],con[128],addr[256],ph[32],act[8];
            SQLLEN len;
            SQLGetData(hStmt,1,SQL_C_CHAR,id,  sizeof(id),  &len); out.id           = atoi(id);
            SQLGetData(hStmt,2,SQL_C_CHAR,fn,  sizeof(fn),  &len); out.fullName     = fn;
            SQLGetData(hStmt,3,SQL_C_CHAR,nidv,sizeof(nidv),&len); out.nidNumber    = nidv;
            SQLGetData(hStmt,4,SQL_C_CHAR,dob, sizeof(dob), &len); out.dob          = dob;
            SQLGetData(hStmt,5,SQL_C_CHAR,con, sizeof(con), &len); out.constituency = con;
            SQLGetData(hStmt,6,SQL_C_CHAR,addr,sizeof(addr),&len); out.address      = (len==SQL_NULL_DATA)?"":addr;
            SQLGetData(hStmt,7,SQL_C_CHAR,ph,  sizeof(ph),  &len); out.phone        = (len==SQL_NULL_DATA)?"":ph;
            SQLGetData(hStmt,8,SQL_C_CHAR,act, sizeof(act), &len); out.isActive     = atoi(act);
            found = true;
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return found;
}

std::vector<Voter> DatabaseManager::getAllVoters() {
    std::vector<Voter> list;
    SQLHSTMT hStmt; SQLAllocHandle(SQL_HANDLE_STMT,hDbc,&hStmt);
    std::string sql = "SELECT voter_id,full_name,nid_number,date_of_birth,constituency,phone,is_active FROM voters ORDER BY full_name";
    if (SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)!=SQL_SUCCESS &&
        SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)!=SQL_SUCCESS_WITH_INFO) {
        SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return list;
    }
    char id[16],fn[128],nid[32],dob[32],con[128],ph[32],act[8]; SQLLEN len;
    while (SQLFetch(hStmt)==SQL_SUCCESS) {
        Voter v;
        SQLGetData(hStmt,1,SQL_C_CHAR,id, sizeof(id), &len); v.id           = atoi(id);
        SQLGetData(hStmt,2,SQL_C_CHAR,fn, sizeof(fn), &len); v.fullName     = fn;
        SQLGetData(hStmt,3,SQL_C_CHAR,nid,sizeof(nid),&len); v.nidNumber    = nid;
        SQLGetData(hStmt,4,SQL_C_CHAR,dob,sizeof(dob),&len); v.dob          = dob;
        SQLGetData(hStmt,5,SQL_C_CHAR,con,sizeof(con),&len); v.constituency = con;
        SQLGetData(hStmt,6,SQL_C_CHAR,ph, sizeof(ph), &len); v.phone        = (len==SQL_NULL_DATA)?"":ph;
        SQLGetData(hStmt,7,SQL_C_CHAR,act,sizeof(act),&len); v.isActive     = atoi(act);
        list.push_back(v);
    }
    SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return list;
}

bool DatabaseManager::removeVoter(int voterId) {
    std::string sql = "UPDATE voters SET is_active=0 WHERE voter_id=" + std::to_string(voterId);
    return executeNonQuery(sql);
}

bool DatabaseManager::voterExists(const std::string& nid) {
    SQLHSTMT hStmt; SQLAllocHandle(SQL_HANDLE_STMT,hDbc,&hStmt);
    std::string sql = "SELECT COUNT(*) FROM voters WHERE nid_number='" + nid + "'";
    bool exists = false;
    if (SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)==SQL_SUCCESS ||
        SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)==SQL_SUCCESS_WITH_INFO) {
        char cnt[16]; SQLLEN len;
        if (SQLFetch(hStmt)==SQL_SUCCESS) {
            SQLGetData(hStmt,1,SQL_C_CHAR,cnt,sizeof(cnt),&len);
            exists = atoi(cnt) > 0;
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return exists;
}

bool DatabaseManager::hasVoted(int voterId, int electionId) {
    SQLHSTMT hStmt; SQLAllocHandle(SQL_HANDLE_STMT,hDbc,&hStmt);
    std::string sql = "SELECT COUNT(*) FROM votes WHERE voter_id=" + std::to_string(voterId) +
                      " AND election_id=" + std::to_string(electionId);
    bool voted = false;
    if (SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)==SQL_SUCCESS ||
        SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)==SQL_SUCCESS_WITH_INFO) {
        char cnt[16]; SQLLEN len;
        if (SQLFetch(hStmt)==SQL_SUCCESS) {
            SQLGetData(hStmt,1,SQL_C_CHAR,cnt,sizeof(cnt),&len);
            voted = atoi(cnt) > 0;
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return voted;
}

// ============================================================
//  Cast Vote
// ============================================================
bool DatabaseManager::castVote(int voterId, int electionId, int candidateId, const std::string& ip) {
    if (hasVoted(voterId, electionId)) return false;
    std::string sql = "INSERT INTO votes (voter_id,election_id,candidate_id,ip_address) VALUES (" +
                      std::to_string(voterId) + "," + std::to_string(electionId) + "," +
                      std::to_string(candidateId) + ",'" + ip + "')";
    return executeNonQuery(sql);
}

// ============================================================
//  Results
// ============================================================
std::vector<VoteResult> DatabaseManager::getResults(int electionId) {
    std::vector<VoteResult> list;
    SQLHSTMT hStmt; SQLAllocHandle(SQL_HANDLE_STMT,hDbc,&hStmt);
    std::string sql = "SELECT candidate_id,candidate_name,party_name,candidate_constituency,"
                      "total_votes,vote_percentage FROM v_vote_results WHERE election_id=" +
                      std::to_string(electionId);
    if (SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)!=SQL_SUCCESS &&
        SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)!=SQL_SUCCESS_WITH_INFO) {
        SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return list;
    }
    char cid[16],cn[128],party[128],con[128],tv[16],vp[16]; SQLLEN len;
    while (SQLFetch(hStmt)==SQL_SUCCESS) {
        VoteResult r;
        SQLGetData(hStmt,1,SQL_C_CHAR,cid,  sizeof(cid),  &len); r.candidateId   = atoi(cid);
        SQLGetData(hStmt,2,SQL_C_CHAR,cn,   sizeof(cn),   &len); r.candidateName = cn;
        SQLGetData(hStmt,3,SQL_C_CHAR,party,sizeof(party),&len); r.partyName     = party;
        SQLGetData(hStmt,4,SQL_C_CHAR,con,  sizeof(con),  &len); r.constituency  = con;
        SQLGetData(hStmt,5,SQL_C_CHAR,tv,   sizeof(tv),   &len); r.totalVotes    = atoi(tv);
        SQLGetData(hStmt,6,SQL_C_CHAR,vp,   sizeof(vp),   &len); r.percentage    = atof(vp);
        list.push_back(r);
    }
    SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return list;
}

int DatabaseManager::getTotalVotesCast(int electionId) {
    SQLHSTMT hStmt; SQLAllocHandle(SQL_HANDLE_STMT,hDbc,&hStmt);
    std::string sql = "SELECT COUNT(*) FROM votes WHERE election_id=" + std::to_string(electionId);
    int count = 0;
    if (SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)==SQL_SUCCESS ||
        SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)==SQL_SUCCESS_WITH_INFO) {
        char cnt[16]; SQLLEN len;
        if (SQLFetch(hStmt)==SQL_SUCCESS) {
            SQLGetData(hStmt,1,SQL_C_CHAR,cnt,sizeof(cnt),&len);
            count = atoi(cnt);
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return count;
}

// ============================================================
//  Audit / Anti-Cheat
// ============================================================
void DatabaseManager::logEvent(const std::string& eventType, const std::string& userType,
                               int userId, const std::string& desc, const std::string& ip) {
    std::string sql = "INSERT INTO audit_log (event_type,user_type,user_id,description,ip_address) "
                      "VALUES ('" + eventType + "','" + userType + "'," + std::to_string(userId) +
                      ",'" + desc + "','" + ip + "')";
    executeNonQuery(sql);
}

std::vector<AuditEntry> DatabaseManager::getAuditLog(int limit) {
    std::vector<AuditEntry> list;
    SQLHSTMT hStmt; SQLAllocHandle(SQL_HANDLE_STMT,hDbc,&hStmt);
    std::string sql = "SELECT log_id,event_type,user_type,COALESCE(description,''),COALESCE(ip_address,''),logged_at "
                      "FROM audit_log ORDER BY log_id DESC LIMIT " + std::to_string(limit);
    if (SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)!=SQL_SUCCESS &&
        SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)!=SQL_SUCCESS_WITH_INFO) {
        SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return list;
    }
    char id[16],et[64],ut[16],desc[512],ip[64],ts[32]; SQLLEN len;
    while (SQLFetch(hStmt)==SQL_SUCCESS) {
        AuditEntry e;
        SQLGetData(hStmt,1,SQL_C_CHAR,id,  sizeof(id),  &len); e.logId       = atoi(id);
        SQLGetData(hStmt,2,SQL_C_CHAR,et,  sizeof(et),  &len); e.eventType   = et;
        SQLGetData(hStmt,3,SQL_C_CHAR,ut,  sizeof(ut),  &len); e.userType    = ut;
        SQLGetData(hStmt,4,SQL_C_CHAR,desc,sizeof(desc),&len); e.description = desc;
        SQLGetData(hStmt,5,SQL_C_CHAR,ip,  sizeof(ip),  &len); e.ipAddress   = ip;
        SQLGetData(hStmt,6,SQL_C_CHAR,ts,  sizeof(ts),  &len); e.loggedAt    = ts;
        list.push_back(e);
    }
    SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return list;
}

std::vector<AuditEntry> DatabaseManager::getSuspiciousActivity() {
    std::vector<AuditEntry> list;
    SQLHSTMT hStmt; SQLAllocHandle(SQL_HANDLE_STMT,hDbc,&hStmt);
    // Votes from same IP (>1 vote per IP per election) or failed logins
    std::string sql =
        "SELECT 0 AS log_id, 'MULTI_IP_VOTE' AS event_type, 'VOTER' AS user_type, "
        "CONCAT('IP ',ip_address,' cast ',COUNT(*),' votes in election #',election_id) AS description, "
        "ip_address, MAX(voted_at) AS logged_at "
        "FROM votes "
        "GROUP BY ip_address, election_id "
        "HAVING COUNT(*) > 1 "
        "UNION ALL "
        "SELECT 0,'FAILED_LOGIN','VOTER', "
        "CONCAT(COUNT(*),' failed logins for NID ',nid_number,' in last hour'), "
        "COALESCE(ip_address,''), MAX(attempt_time) "
        "FROM login_attempts WHERE success=0 AND attempt_time > DATE_SUB(NOW(), INTERVAL 1 HOUR) "
        "GROUP BY nid_number, ip_address HAVING COUNT(*) >= 3 "
        "ORDER BY logged_at DESC";
    if (SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)!=SQL_SUCCESS &&
        SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)!=SQL_SUCCESS_WITH_INFO) {
        SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return list;
    }
    char id[16],et[64],ut[16],desc[512],ip[64],ts[32]; SQLLEN len;
    while (SQLFetch(hStmt)==SQL_SUCCESS) {
        AuditEntry e;
        SQLGetData(hStmt,1,SQL_C_CHAR,id,  sizeof(id),  &len); e.logId       = atoi(id);
        SQLGetData(hStmt,2,SQL_C_CHAR,et,  sizeof(et),  &len); e.eventType   = et;
        SQLGetData(hStmt,3,SQL_C_CHAR,ut,  sizeof(ut),  &len); e.userType    = ut;
        SQLGetData(hStmt,4,SQL_C_CHAR,desc,sizeof(desc),&len); e.description = desc;
        SQLGetData(hStmt,5,SQL_C_CHAR,ip,  sizeof(ip),  &len); e.ipAddress   = ip;
        SQLGetData(hStmt,6,SQL_C_CHAR,ts,  sizeof(ts),  &len); e.loggedAt    = ts;
        list.push_back(e);
    }
    SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return list;
}

void DatabaseManager::recordLoginAttempt(const std::string& nid, bool success) {
    std::string sql = "INSERT INTO login_attempts (nid_number,success) VALUES ('" +
                      nid + "'," + (success?"1":"0") + ")";
    executeNonQuery(sql);
}

int DatabaseManager::getFailedLoginCount(const std::string& nid, int minutes) {
    SQLHSTMT hStmt; SQLAllocHandle(SQL_HANDLE_STMT,hDbc,&hStmt);
    std::string sql = "SELECT COUNT(*) FROM login_attempts WHERE nid_number='" + nid +
                      "' AND success=0 AND attempt_time > DATE_SUB(NOW(), INTERVAL " +
                      std::to_string(minutes) + " MINUTE)";
    int count = 0;
    if (SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)==SQL_SUCCESS ||
        SQLExecDirect(hStmt,(SQLCHAR*)sql.c_str(),SQL_NTS)==SQL_SUCCESS_WITH_INFO) {
        char cnt[16]; SQLLEN len;
        if (SQLFetch(hStmt)==SQL_SUCCESS) {
            SQLGetData(hStmt,1,SQL_C_CHAR,cnt,sizeof(cnt),&len);
            count = atoi(cnt);
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT,hStmt); return count;
}
