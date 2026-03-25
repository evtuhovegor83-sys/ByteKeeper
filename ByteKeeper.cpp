#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <regex>
#include <string>
#include <cwchar>
#include <iomanip>
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

// Функция просмотра всех файлов (с динамической шириной)
// Функция просмотра всех файлов (с динамической шириной)
void viewAllFiles() {
    SQLHSTMT stmt = db.getStatement();

    SQLWCHAR query[] = L"SELECT r.ResourceID, r.Name, r.Size, c.CategoryName, u.UserName, r.isDeleted "
        L"FROM Resources r "
        L"JOIN Categories c ON r.CategoryID = c.CategoryID "
        L"JOIN Users u ON r.OwnerID = u.UserID";

    SQLRETURN ret = SQLExecDirectW(stmt, query, SQL_NTS);

    if (ret != SQL_SUCCESS) {
        cout << "Ошибка при выполнении запроса.\n";
        return;
    }

    // Сначала проходим по данным, чтобы узнать максимальную длину имени
    int maxNameLen = 15;
    int maxOwnerLen = 15;

    while (SQLFetch(stmt) == SQL_SUCCESS) {
        SQLWCHAR name[256];
        SQLWCHAR owner[256];
        SQLGetData(stmt, 2, SQL_C_WCHAR, name, sizeof(name), NULL);
        SQLGetData(stmt, 5, SQL_C_WCHAR, owner, sizeof(owner), NULL);

        int nameLen = wcslen(name);
        int ownerLen = wcslen(owner);
        if (nameLen > maxNameLen) maxNameLen = min(nameLen, 30);
        if (ownerLen > maxOwnerLen) maxOwnerLen = min(ownerLen, 30);
    }

    SQLFreeStmt(stmt, SQL_CLOSE);

    // Повторно выполняем запрос для вывода
    ret = SQLExecDirectW(stmt, query, SQL_NTS);
    if (ret != SQL_SUCCESS) {
        cout << "Ошибка при выполнении запроса.\n";
        return;
    }

    // Заголовок таблицы
    cout << "\n";
    cout << "+" << string(maxNameLen + 2, '-') << "+" << string(15, '-') << "+" << string(maxOwnerLen + 2, '-') << "+" << string(20, '-') << "+" << string(12, '-') << "+\n";
    cout << "| " << left << setw(maxNameLen) << "Имя файла"
        << " | " << setw(13) << "Размер (байт)"
        << " | " << setw(maxOwnerLen) << "Владелец"
        << " | " << setw(18) << "Категория"
        << " | " << setw(10) << "Статус" << " |\n";
    cout << "+" << string(maxNameLen + 2, '-') << "+" << string(15, '-') << "+" << string(maxOwnerLen + 2, '-') << "+" << string(20, '-') << "+" << string(12, '-') << "+\n";

    int count = 0;
    while (SQLFetch(stmt) == SQL_SUCCESS) {
        int id;
        SQLWCHAR name[256];
        long long size;
        SQLWCHAR category[256];
        SQLWCHAR owner[256];
        int isDeleted;

        SQLGetData(stmt, 1, SQL_C_LONG, &id, 0, NULL);
        SQLGetData(stmt, 2, SQL_C_WCHAR, name, sizeof(name), NULL);
        SQLGetData(stmt, 3, SQL_C_SLONG, &size, 0, NULL);
        SQLGetData(stmt, 4, SQL_C_WCHAR, category, sizeof(category), NULL);
        SQLGetData(stmt, 5, SQL_C_WCHAR, owner, sizeof(owner), NULL);
        SQLGetData(stmt, 6, SQL_C_LONG, &isDeleted, 0, NULL);

        // Преобразуем широкие строки в обычные
        char nameBuf[256];
        char ownerBuf[256];
        wcstombs(nameBuf, name, 256);
        wcstombs(ownerBuf, owner, 256);

        string strName(nameBuf);
        string strOwner(ownerBuf);
        string strCategory;
        wcstombs(nullptr, category, 0); // упрощённо, категория выводится как есть

        // Обрезаем длинные имена
        if (strName.length() > maxNameLen) strName = strName.substr(0, maxNameLen - 3) + "...";
        if (strOwner.length() > maxOwnerLen) strOwner = strOwner.substr(0, maxOwnerLen - 3) + "...";

        string status = (isDeleted == 1) ? "В корзине" : "Активен";

        // Выводим категорию
        char categoryBuf[256];
        wcstombs(categoryBuf, category, 256);

        cout << "| " << left << setw(maxNameLen) << strName
            << " | " << setw(13) << size
            << " | " << setw(maxOwnerLen) << strOwner
            << " | " << setw(18) << categoryBuf
            << " | " << setw(10) << status << " |\n";
        count++;
    }

    cout << "+" << string(maxNameLen + 2, '-') << "+" << string(15, '-') << "+" << string(maxOwnerLen + 2, '-') << "+" << string(20, '-') << "+" << string(12, '-') << "+\n";
    cout << "Всего записей: " << count << "\n";

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
        case 2:
            viewAllFiles();
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