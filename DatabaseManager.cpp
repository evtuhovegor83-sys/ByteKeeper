#include "DatabaseManager.h"
#include <iostream>

DatabaseManager::DatabaseManager() : hEnv(nullptr), hDbc(nullptr), hStmt(nullptr), connected(false) {
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
}

DatabaseManager::~DatabaseManager() {
    disconnect();
    if (hDbc) SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}

void DatabaseManager::showError(SQLSMALLINT handleType, SQLHANDLE handle) {
    SQLWCHAR sqlState[6], message[256];
    SQLINTEGER nativeError;
    SQLSMALLINT textLen;

    if (SQLGetDiagRecW(handleType, handle, 1, sqlState, &nativeError, message, sizeof(message), &textLen) == SQL_SUCCESS) {
        wchar_t buf[512];
        swprintf(buf, 512, L"Ошибка SQL: %s (SQLSTATE: %s)", message, sqlState);
        std::wcerr << buf << std::endl;
    }
}

bool DatabaseManager::connect(const std::string& server, const std::string& database) {
    // Используем широкие строки для подключения
    std::wstring connStr = L"DRIVER={SQL Server};SERVER=" + std::wstring(server.begin(), server.end()) +
        L";DATABASE=" + std::wstring(database.begin(), database.end()) +
        L";Trusted_Connection=yes;";

    SQLRETURN ret = SQLDriverConnectW(hDbc, NULL, (SQLWCHAR*)connStr.c_str(), SQL_NTS,
        NULL, 0, NULL, SQL_DRIVER_COMPLETE);

    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        connected = true;
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
        std::cout << "Подключение к базе данных " << database << " успешно!\n";
        return true;
    }
    else {
        showError(SQL_HANDLE_DBC, hDbc);
        return false;
    }
}

void DatabaseManager::disconnect() {
    if (hStmt) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        hStmt = nullptr;
    }
    if (hDbc && connected) {
        SQLDisconnect(hDbc);
        connected = false;
        std::cout << "Отключено от базы данных.\n";
    }
}

bool DatabaseManager::isConnected() const {
    return connected;
}