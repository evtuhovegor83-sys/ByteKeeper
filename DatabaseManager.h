#pragma once
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <string>

class DatabaseManager {
private:
    SQLHENV hEnv;      // Окружение ODBC
    SQLHDBC hDbc;      // Соединение с БД
    SQLHSTMT hStmt;    // Дескриптор запроса
    bool connected;

    void showError(SQLSMALLINT handleType, SQLHANDLE handle);

public:
    DatabaseManager();
    ~DatabaseManager();

    bool connect(const std::string& server, const std::string& database);
    void disconnect();
    bool isConnected() const;

    SQLHDBC getConnection() const { return hDbc; }
    SQLHSTMT getStatement() const { return hStmt; }
};