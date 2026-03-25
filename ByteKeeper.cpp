#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <regex>
#include <string>
#include <cwchar>
#include "DatabaseManager.h"

using namespace std;

DatabaseManager db;

void showMenu() {
    cout << "\n========================================\n";
    cout << "   СИСТЕМА УПРАВЛЕНИЯ ЦИФРОВЫМИ АКТИВАМИ\n";
    cout << "========================================\n";
    cout << "1. Добавить файл\n";
    cout << "2. Просмотр всех файлов\n";
    cout << "3. Поиск по имени\n";
    cout << "4. Удаление (в корзину)\n";
    cout << "5. Восстановление из корзины\n";
    cout << "6. Статистика\n";
    cout << "7. Очистка старых записей\n";
    cout << "8. Экспорт в CSV\n";
    cout << "9. Показать логи\n";
    cout << "0. Выход\n";
    cout << "========================================\n";
    cout << "Ваш выбор: ";
}

// Функция добавления файла
void addFile() {
    string name, categoryName, ownerName;
    long long size;
    int categoryID = 0, ownerID = 0;

    cout << "Введите имя файла: ";
    cin >> name;
    cout << "Введите размер (байт): ";
    cin >> size;
    cout << "Введите категорию (Документы, Медиа, Архивы): ";
    cin >> categoryName;
    cout << "Введите имя владельца (Администратор, Иванов, Петров): ";
    cin >> ownerName;

    // Проверка имени на запрещённые символы
    regex namePattern(R"(^[a-zA-Z0-9_\-\.]+$)");
    if (!regex_match(name, namePattern)) {
        cout << "Ошибка! Имя содержит запрещённые символы.\n";
        return;
    }

    // Проверка расширения (белый список)
    regex extPattern(R"(\.(txt|pdf|docx|exe|jpg|png)$)");
    if (!regex_match(name, extPattern)) {
        cout << "Ошибка! Разрешены только: .txt, .pdf, .docx, .exe, .jpg, .png\n";
        return;
    }

    SQLHSTMT stmt = db.getStatement();
    SQLRETURN ret;

    // Получаем CategoryID
    SQLWCHAR queryCategory[] = L"SELECT CategoryID FROM Categories WHERE CategoryName = ?";
    ret = SQLPrepareW(stmt, queryCategory, SQL_NTS);

    SQLWCHAR categoryParam[100];
    mbstowcs((wchar_t*)categoryParam, categoryName.c_str(), 100);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 100, 0, categoryParam, 0, NULL);

    ret = SQLExecute(stmt);
    if (ret == SQL_SUCCESS) {
        SQLFetch(stmt);
        SQLGetData(stmt, 1, SQL_C_LONG, &categoryID, 0, NULL);
    }
    SQLFreeStmt(stmt, SQL_CLOSE);

    // Получаем OwnerID
    SQLWCHAR queryOwner[] = L"SELECT UserID FROM Users WHERE UserName = ?";
    ret = SQLPrepareW(stmt, queryOwner, SQL_NTS);

    SQLWCHAR ownerParam[100];
    mbstowcs((wchar_t*)ownerParam, ownerName.c_str(), 100);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 100, 0, ownerParam, 0, NULL);

    ret = SQLExecute(stmt);
    if (ret == SQL_SUCCESS) {
        SQLFetch(stmt);
        SQLGetData(stmt, 1, SQL_C_LONG, &ownerID, 0, NULL);
    }
    SQLFreeStmt(stmt, SQL_CLOSE);

    if (categoryID == 0 || ownerID == 0) {
        cout << "Ошибка! Категория или владелец не найдены.\n";
        return;
    }

    // Проверка дубликатов
    SQLWCHAR queryCheck[] = L"SELECT ResourceID FROM Resources WHERE Name = ?";
    ret = SQLPrepareW(stmt, queryCheck, SQL_NTS);

    SQLWCHAR nameParam[255];
    mbstowcs((wchar_t*)nameParam, name.c_str(), 255);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, nameParam, 0, NULL);

    ret = SQLExecute(stmt);
    int exists = 0;
    if (ret == SQL_SUCCESS) {
        if (SQLFetch(stmt) == SQL_SUCCESS) {
            exists = 1;
        }
    }
    SQLFreeStmt(stmt, SQL_CLOSE);

    if (exists) {
        cout << "Ошибка! Файл с таким именем уже существует.\n";
        return;
    }

    // INSERT
    SQLWCHAR queryInsert[] = L"INSERT INTO Resources (Name, Size, CategoryID, OwnerID) VALUES (?, ?, ?, ?)";
    ret = SQLPrepareW(stmt, queryInsert, SQL_NTS);

    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, nameParam, 0, NULL);
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &size, 0, NULL);
    SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &categoryID, 0, NULL);
    SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &ownerID, 0, NULL);

    ret = SQLExecute(stmt);
    if (ret == SQL_SUCCESS) {
        cout << "Файл успешно добавлен!\n";

        // Логирование
        SQLWCHAR queryLog[] = L"INSERT INTO Logs (Action) VALUES (?)";
        SQLPrepareW(stmt, queryLog, SQL_NTS);
        wstring logMsg = L"Добавлен файл: " + wstring(nameParam);
        SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, (SQLWCHAR*)logMsg.c_str(), 0, NULL);
        SQLExecute(stmt);
    }
    else {
        cout << "Ошибка при добавлении файла.\n";
    }
    SQLFreeStmt(stmt, SQL_CLOSE);
}

int main() {
    setlocale(LC_ALL, "Russian");

    cout << "=== СИСТЕМА УПРАВЛЕНИЯ ЦИФРОВЫМИ АКТИВАМИ ===\n";

    string server, database;
    cout << "Введите имя сервера (например, localhost или .\\SQLEXPRESS): ";
    cin >> server;
    cout << "Введите имя базы данных: ";
    cin >> database;

    if (!db.connect(server, database)) {
        cout << "Не удалось подключиться к базе данных. Программа завершена.\n";
        return 1;
    }

    cout << "Добро пожаловать!\n";

    int choice;
    do {
        showMenu();
        cin >> choice;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Ошибка! Введите число.\n";
            continue;
        }

        switch (choice) {
        case 1:
            addFile();
            break;
        case 0:
            cout << "Выход из программы...\n";
            break;
        default:
            cout << "Функция будет реализована позже...\n";
        }
    } while (choice != 0);

    return 0;
}