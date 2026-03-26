#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <regex>
#include <string>
#include <cwchar>
#include <iomanip>
#include <fstream>
#include <windows.h>
#include "DatabaseManager.h"

using namespace std;

DatabaseManager db;

// Функция для установки цвета текста
void setColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

// Цвета
#define COLOR_DEFAULT 7      // белый на черном
#define COLOR_GREEN 10       // зеленый (активные файлы)
#define COLOR_RED 12         // красный (в корзине)
#define COLOR_YELLOW 14      // желтый (предупреждения)
#define COLOR_CYAN 11        // голубой (заголовки)

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
    cout << "10. Просмотр с пагинацией\n";
    cout << "11. Проверить состояние сервера\n";
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
        setColor(COLOR_RED);
        cout << "Ошибка! Имя содержит запрещённые символы.\n";
        setColor(COLOR_DEFAULT);
        return;
    }

    // Проверка расширения (белый список)
    regex extPattern(R"(\.(txt|pdf|docx|exe|jpg|png)$)");
    if (!regex_match(name, extPattern)) {
        setColor(COLOR_RED);
        cout << "Ошибка! Разрешены только: .txt, .pdf, .docx, .exe, .jpg, .png\n";
        setColor(COLOR_DEFAULT);
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
        setColor(COLOR_RED);
        cout << "Ошибка! Категория или владелец не найдены.\n";
        setColor(COLOR_DEFAULT);
        return;
    }

    // Проверка дубликатов
    SQLWCHAR queryCheck[] = L"SELECT ResourceID FROM Resources WHERE Name = ? AND isDeleted = 0";
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
        setColor(COLOR_RED);
        cout << "Ошибка! Файл с таким именем уже существует.\n";
        setColor(COLOR_DEFAULT);
        return;
    }

    // INSERT
    SQLWCHAR queryInsert[] = L"INSERT INTO Resources (Name, Size, CategoryID, OwnerID, isDeleted, CreatedDate) VALUES (?, ?, ?, ?, 0, GETDATE())";
    ret = SQLPrepareW(stmt, queryInsert, SQL_NTS);

    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, nameParam, 0, NULL);
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &size, 0, NULL);
    SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &categoryID, 0, NULL);
    SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &ownerID, 0, NULL);

    ret = SQLExecute(stmt);
    if (ret == SQL_SUCCESS) {
        setColor(COLOR_GREEN);
        cout << "Файл успешно добавлен!\n";
        setColor(COLOR_DEFAULT);

        // Логирование
        SQLWCHAR queryLog[] = L"INSERT INTO Logs (Action) VALUES (?)";
        SQLPrepareW(stmt, queryLog, SQL_NTS);
        wstring logMsg = L"Добавлен файл: " + wstring(nameParam);
        SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, (SQLWCHAR*)logMsg.c_str(), 0, NULL);
        SQLExecute(stmt);
    }
    else {
        setColor(COLOR_RED);
        cout << "Ошибка при добавлении файла.\n";
        setColor(COLOR_DEFAULT);
    }
    SQLFreeStmt(stmt, SQL_CLOSE);
}

// Функция просмотра всех файлов (с динамической шириной и цветом)
void viewAllFiles() {
    SQLHSTMT stmt = db.getStatement();

    SQLWCHAR query[] = L"SELECT r.ResourceID, r.Name, r.Size, c.CategoryName, u.UserName, r.isDeleted "
        L"FROM Resources r "
        L"JOIN Categories c ON r.CategoryID = c.CategoryID "
        L"JOIN Users u ON r.OwnerID = u.UserID";

    SQLRETURN ret = SQLExecDirectW(stmt, query, SQL_NTS);

    if (ret != SQL_SUCCESS) {
        setColor(COLOR_RED);
        cout << "Ошибка при выполнении запроса.\n";
        setColor(COLOR_DEFAULT);
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
        setColor(COLOR_RED);
        cout << "Ошибка при выполнении запроса.\n";
        setColor(COLOR_DEFAULT);
        return;
    }

    // Заголовок таблицы (голубым цветом)
    setColor(COLOR_CYAN);
    cout << "\n";
    cout << "+" << string(maxNameLen + 2, '-') << "+" << string(15, '-') << "+" << string(maxOwnerLen + 2, '-') << "+" << string(20, '-') << "+" << string(12, '-') << "+\n";
    cout << "| " << left << setw(maxNameLen) << "Имя файла"
        << " | " << setw(13) << "Размер (байт)"
        << " | " << setw(maxOwnerLen) << "Владелец"
        << " | " << setw(18) << "Категория"
        << " | " << setw(10) << "Статус" << " |\n";
    cout << "+" << string(maxNameLen + 2, '-') << "+" << string(15, '-') << "+" << string(maxOwnerLen + 2, '-') << "+" << string(20, '-') << "+" << string(12, '-') << "+\n";
    setColor(COLOR_DEFAULT);

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

        // Обрезаем длинные имена
        if (strName.length() > maxNameLen) strName = strName.substr(0, maxNameLen - 3) + "...";
        if (strOwner.length() > maxOwnerLen) strOwner = strOwner.substr(0, maxOwnerLen - 3) + "...";

        string status = (isDeleted == 1) ? "В корзине" : "Активен";

        // Выводим категорию
        char categoryBuf[256];
        wcstombs(categoryBuf, category, 256);

        // Устанавливаем цвет в зависимости от статуса
        if (isDeleted == 1) {
            setColor(COLOR_RED);  // красный для корзины
        }
        else {
            setColor(COLOR_GREEN); // зеленый для активных
        }

        cout << "| " << left << setw(maxNameLen) << strName
            << " | " << setw(13) << size
            << " | " << setw(maxOwnerLen) << strOwner
            << " | " << setw(18) << categoryBuf
            << " | " << setw(10) << status << " |\n";
        count++;
    }

    setColor(COLOR_DEFAULT);
    cout << "+" << string(maxNameLen + 2, '-') << "+" << string(15, '-') << "+" << string(maxOwnerLen + 2, '-') << "+" << string(20, '-') << "+" << string(12, '-') << "+\n";
    cout << "Всего записей: " << count << "\n";

    SQLFreeStmt(stmt, SQL_CLOSE);
}

// Функция поиска по имени (LIKE) с цветом
void searchByName() {
    string mask;
    cout << "Введите часть имени для поиска: ";
    cin >> mask;

    SQLHSTMT stmt = db.getStatement();

    SQLWCHAR query[] = L"SELECT r.ResourceID, r.Name, r.Size, c.CategoryName, u.UserName, r.isDeleted "
        L"FROM Resources r "
        L"JOIN Categories c ON r.CategoryID = c.CategoryID "
        L"JOIN Users u ON r.OwnerID = u.UserID "
        L"WHERE r.Name LIKE ? AND r.isDeleted = 0";

    SQLRETURN ret = SQLPrepareW(stmt, query, SQL_NTS);

    string searchPattern = "%" + mask + "%";
    SQLWCHAR searchParam[256];
    mbstowcs((wchar_t*)searchParam, searchPattern.c_str(), 256);

    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, searchParam, 0, NULL);

    ret = SQLExecute(stmt);

    if (ret != SQL_SUCCESS) {
        setColor(COLOR_RED);
        cout << "Ошибка при выполнении поиска.\n";
        setColor(COLOR_DEFAULT);
        SQLFreeStmt(stmt, SQL_CLOSE);
        return;
    }

    // Заголовок таблицы (голубым)
    setColor(COLOR_CYAN);
    cout << "\n";
    cout << "+" << string(25, '-') << "+" << string(15, '-') << "+" << string(20, '-') << "+" << string(20, '-') << "+" << string(12, '-') << "+\n";
    cout << "| " << left << setw(23) << "Имя файла"
        << " | " << setw(13) << "Размер (байт)"
        << " | " << setw(18) << "Владелец"
        << " | " << setw(18) << "Категория"
        << " | " << setw(10) << "Статус" << " |\n";
    cout << "+" << string(25, '-') << "+" << string(15, '-') << "+" << string(20, '-') << "+" << string(20, '-') << "+" << string(12, '-') << "+\n";
    setColor(COLOR_DEFAULT);

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

        // Преобразуем широкие строки
        char nameBuf[256], ownerBuf[256], categoryBuf[256];
        wcstombs(nameBuf, name, 256);
        wcstombs(ownerBuf, owner, 256);
        wcstombs(categoryBuf, category, 256);

        string strName(nameBuf);
        string strOwner(ownerBuf);

        if (strName.length() > 23) strName = strName.substr(0, 20) + "...";
        if (strOwner.length() > 18) strOwner = strOwner.substr(0, 15) + "...";

        string status = (isDeleted == 1) ? "В корзине" : "Активен";

        setColor(COLOR_GREEN);
        cout << "| " << left << setw(23) << strName
            << " | " << setw(13) << size
            << " | " << setw(18) << strOwner
            << " | " << setw(18) << categoryBuf
            << " | " << setw(10) << status << " |\n";
        count++;
    }

    setColor(COLOR_DEFAULT);
    cout << "+" << string(25, '-') << "+" << string(15, '-') << "+" << string(20, '-') << "+" << string(20, '-') << "+" << string(12, '-') << "+\n";

    if (count == 0) {
        setColor(COLOR_YELLOW);
        cout << "Ничего не найдено.\n";
        setColor(COLOR_DEFAULT);
    }
    else {
        cout << "Найдено записей: " << count << "\n";
    }

    SQLFreeStmt(stmt, SQL_CLOSE);
}

// Функция статистики (COUNT/SUM) с цветом
void showStatistics() {
    SQLHSTMT stmt = db.getStatement();

    SQLWCHAR query[] = L"SELECT "
        L"COUNT(*) as TotalFiles, "
        L"SUM(Size) as TotalSize "
        L"FROM Resources "
        L"WHERE isDeleted = 0";

    SQLRETURN ret = SQLExecDirectW(stmt, query, SQL_NTS);

    if (ret != SQL_SUCCESS) {
        setColor(COLOR_RED);
        cout << "Ошибка при выполнении запроса статистики.\n";
        setColor(COLOR_DEFAULT);
        SQLFreeStmt(stmt, SQL_CLOSE);
        return;
    }

    int totalFiles = 0;
    long long totalSize = 0;

    if (SQLFetch(stmt) == SQL_SUCCESS) {
        SQLGetData(stmt, 1, SQL_C_LONG, &totalFiles, 0, NULL);
        SQLGetData(stmt, 2, SQL_C_SLONG, &totalSize, 0, NULL);
    }

    SQLFreeStmt(stmt, SQL_CLOSE);

    setColor(COLOR_CYAN);
    cout << "\n=== СТАТИСТИКА АРХИВА ===\n";
    setColor(COLOR_DEFAULT);
    cout << "Всего файлов: " << totalFiles << "\n";
    cout << "Общий размер: " << totalSize << " байт\n";

    if (totalFiles > 0) {
        cout << "Средний размер: " << (totalSize / totalFiles) << " байт\n";
    }

    SQLWCHAR queryByCategory[] = L"SELECT c.CategoryName, COUNT(r.ResourceID), SUM(r.Size) "
        L"FROM Resources r "
        L"JOIN Categories c ON r.CategoryID = c.CategoryID "
        L"WHERE r.isDeleted = 0 "
        L"GROUP BY c.CategoryName";

    ret = SQLExecDirectW(stmt, queryByCategory, SQL_NTS);

    if (ret == SQL_SUCCESS) {
        setColor(COLOR_CYAN);
        cout << "\n--- По категориям ---\n";
        setColor(COLOR_DEFAULT);
        while (SQLFetch(stmt) == SQL_SUCCESS) {
            SQLWCHAR category[100];
            int count;
            long long size;

            SQLGetData(stmt, 1, SQL_C_WCHAR, category, sizeof(category), NULL);
            SQLGetData(stmt, 2, SQL_C_LONG, &count, 0, NULL);
            SQLGetData(stmt, 3, SQL_C_SLONG, &size, 0, NULL);

            char categoryBuf[100];
            wcstombs(categoryBuf, category, 100);

            cout << "  " << categoryBuf << ": " << count << " файлов, " << size << " байт\n";
        }
    }

    SQLFreeStmt(stmt, SQL_CLOSE);
}

// Функция удаления в корзину (Soft Delete) с цветом
void softDeleteFile() {
    string name;
    cout << "Введите имя файла для удаления: ";
    cin >> name;

    SQLHSTMT stmt = db.getStatement();

    SQLWCHAR queryCheck[] = L"SELECT ResourceID, isDeleted FROM Resources WHERE Name = ?";
    SQLRETURN ret = SQLPrepareW(stmt, queryCheck, SQL_NTS);

    SQLWCHAR nameParam[255];
    mbstowcs((wchar_t*)nameParam, name.c_str(), 255);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, nameParam, 0, NULL);

    ret = SQLExecute(stmt);

    int resourceID = 0;
    int isDeleted = 0;

    if (ret == SQL_SUCCESS && SQLFetch(stmt) == SQL_SUCCESS) {
        SQLGetData(stmt, 1, SQL_C_LONG, &resourceID, 0, NULL);
        SQLGetData(stmt, 2, SQL_C_LONG, &isDeleted, 0, NULL);
    }

    SQLFreeStmt(stmt, SQL_CLOSE);

    if (resourceID == 0) {
        setColor(COLOR_RED);
        cout << "Файл \"" << name << "\" не найден.\n";
        setColor(COLOR_DEFAULT);
        return;
    }

    if (isDeleted == 1) {
        setColor(COLOR_YELLOW);
        cout << "Файл уже находится в корзине.\n";
        setColor(COLOR_DEFAULT);
        return;
    }

    SQLWCHAR queryDelete[] = L"UPDATE Resources SET isDeleted = 1 WHERE Name = ?";
    ret = SQLPrepareW(stmt, queryDelete, SQL_NTS);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, nameParam, 0, NULL);

    ret = SQLExecute(stmt);

    if (ret == SQL_SUCCESS) {
        setColor(COLOR_YELLOW);
        cout << "Файл \"" << name << "\" перемещён в корзину.\n";
        setColor(COLOR_DEFAULT);

        SQLWCHAR queryLog[] = L"INSERT INTO Logs (Action) VALUES (?)";
        SQLPrepareW(stmt, queryLog, SQL_NTS);
        wstring logMsg = L"Удалён в корзину: " + wstring(nameParam);
        SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, (SQLWCHAR*)logMsg.c_str(), 0, NULL);
        SQLExecute(stmt);
    }
    else {
        setColor(COLOR_RED);
        cout << "Ошибка при удалении файла.\n";
        setColor(COLOR_DEFAULT);
    }

    SQLFreeStmt(stmt, SQL_CLOSE);
}

// Функция восстановления из корзины с цветом
void restoreFile() {
    string name;
    cout << "Введите имя файла для восстановления: ";
    cin >> name;

    SQLHSTMT stmt = db.getStatement();

    SQLWCHAR queryCheck[] = L"SELECT ResourceID, isDeleted FROM Resources WHERE Name = ?";
    SQLRETURN ret = SQLPrepareW(stmt, queryCheck, SQL_NTS);

    SQLWCHAR nameParam[255];
    mbstowcs((wchar_t*)nameParam, name.c_str(), 255);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, nameParam, 0, NULL);

    ret = SQLExecute(stmt);

    int resourceID = 0;
    int isDeleted = 0;

    if (ret == SQL_SUCCESS && SQLFetch(stmt) == SQL_SUCCESS) {
        SQLGetData(stmt, 1, SQL_C_LONG, &resourceID, 0, NULL);
        SQLGetData(stmt, 2, SQL_C_LONG, &isDeleted, 0, NULL);
    }

    SQLFreeStmt(stmt, SQL_CLOSE);

    if (resourceID == 0) {
        setColor(COLOR_RED);
        cout << "Файл \"" << name << "\" не найден.\n";
        setColor(COLOR_DEFAULT);
        return;
    }

    if (isDeleted == 0) {
        setColor(COLOR_YELLOW);
        cout << "Файл не находится в корзине.\n";
        setColor(COLOR_DEFAULT);
        return;
    }

    SQLWCHAR queryRestore[] = L"UPDATE Resources SET isDeleted = 0 WHERE Name = ?";
    ret = SQLPrepareW(stmt, queryRestore, SQL_NTS);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, nameParam, 0, NULL);

    ret = SQLExecute(stmt);

    if (ret == SQL_SUCCESS) {
        setColor(COLOR_GREEN);
        cout << "Файл \"" << name << "\" восстановлен из корзины.\n";
        setColor(COLOR_DEFAULT);

        SQLWCHAR queryLog[] = L"INSERT INTO Logs (Action) VALUES (?)";
        SQLPrepareW(stmt, queryLog, SQL_NTS);
        wstring logMsg = L"Восстановлен из корзины: " + wstring(nameParam);
        SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, (SQLWCHAR*)logMsg.c_str(), 0, NULL);
        SQLExecute(stmt);
    }
    else {
        setColor(COLOR_RED);
        cout << "Ошибка при восстановлении файла.\n";
        setColor(COLOR_DEFAULT);
    }

    SQLFreeStmt(stmt, SQL_CLOSE);
}

// Функция просмотра логов
void showLogs() {
    SQLHSTMT stmt = db.getStatement();

    SQLWCHAR query[] = L"SELECT LogID, Action, Timestamp FROM Logs ORDER BY Timestamp DESC";
    SQLRETURN ret = SQLExecDirectW(stmt, query, SQL_NTS);

    if (ret != SQL_SUCCESS) {
        setColor(COLOR_RED);
        cout << "Ошибка при выполнении запроса.\n";
        setColor(COLOR_DEFAULT);
        SQLFreeStmt(stmt, SQL_CLOSE);
        return;
    }

    setColor(COLOR_CYAN);
    cout << "\n=== ЖУРНАЛ ДЕЙСТВИЙ ===\n";
    cout << "+" << string(10, '-') << "+" << string(50, '-') << "+" << string(25, '-') << "+\n";
    cout << "| " << left << setw(8) << "ID"
        << " | " << setw(48) << "Действие"
        << " | " << setw(23) << "Время" << " |\n";
    cout << "+" << string(10, '-') << "+" << string(50, '-') << "+" << string(25, '-') << "+\n";
    setColor(COLOR_DEFAULT);

    int count = 0;
    while (SQLFetch(stmt) == SQL_SUCCESS) {
        int logID;
        SQLWCHAR action[512];
        SQLWCHAR timestamp[64];

        SQLGetData(stmt, 1, SQL_C_LONG, &logID, 0, NULL);
        SQLGetData(stmt, 2, SQL_C_WCHAR, action, sizeof(action), NULL);
        SQLGetData(stmt, 3, SQL_C_WCHAR, timestamp, sizeof(timestamp), NULL);

        char actionBuf[512], timeBuf[64];
        wcstombs(actionBuf, action, 512);
        wcstombs(timeBuf, timestamp, 64);

        cout << "| " << setw(8) << logID
            << " | " << setw(48) << actionBuf
            << " | " << setw(23) << timeBuf << " |\n";
        count++;
    }

    setColor(COLOR_CYAN);
    cout << "+" << string(10, '-') << "+" << string(50, '-') << "+" << string(25, '-') << "+\n";
    setColor(COLOR_DEFAULT);
    cout << "Всего записей: " << count << "\n";

    SQLFreeStmt(stmt, SQL_CLOSE);
}

// Функция экспорта в CSV
void exportToCSV() {
    string filename;
    cout << "Введите имя файла для экспорта (например, report.csv): ";
    cin >> filename;

    SQLHSTMT stmt = db.getStatement();

    SQLWCHAR query[] = L"SELECT r.Name, r.Size, c.CategoryName, u.UserName, "
        L"CASE WHEN r.isDeleted = 1 THEN 'В корзине' ELSE 'Активен' END as Status, "
        L"r.CreatedDate "
        L"FROM Resources r "
        L"JOIN Categories c ON r.CategoryID = c.CategoryID "
        L"JOIN Users u ON r.OwnerID = u.UserID "
        L"ORDER BY r.ResourceID";

    SQLRETURN ret = SQLExecDirectW(stmt, query, SQL_NTS);

    if (ret != SQL_SUCCESS) {
        setColor(COLOR_RED);
        cout << "Ошибка при выполнении запроса.\n";
        setColor(COLOR_DEFAULT);
        SQLFreeStmt(stmt, SQL_CLOSE);
        return;
    }

    ofstream file(filename);
    if (!file.is_open()) {
        setColor(COLOR_RED);
        cout << "Ошибка при создании файла.\n";
        setColor(COLOR_DEFAULT);
        SQLFreeStmt(stmt, SQL_CLOSE);
        return;
    }

    file << "Имя файла,Размер (байт),Категория,Владелец,Статус,Дата создания\n";

    int count = 0;
    while (SQLFetch(stmt) == SQL_SUCCESS) {
        SQLWCHAR name[256];
        long long size;
        SQLWCHAR category[256];
        SQLWCHAR owner[256];
        SQLWCHAR status[256];
        SQLWCHAR createdDate[64];

        SQLGetData(stmt, 1, SQL_C_WCHAR, name, sizeof(name), NULL);
        SQLGetData(stmt, 2, SQL_C_SLONG, &size, 0, NULL);
        SQLGetData(stmt, 3, SQL_C_WCHAR, category, sizeof(category), NULL);
        SQLGetData(stmt, 4, SQL_C_WCHAR, owner, sizeof(owner), NULL);
        SQLGetData(stmt, 5, SQL_C_WCHAR, status, sizeof(status), NULL);
        SQLGetData(stmt, 6, SQL_C_WCHAR, createdDate, sizeof(createdDate), NULL);

        char nameBuf[256], categoryBuf[256], ownerBuf[256], statusBuf[256], dateBuf[64];
        wcstombs(nameBuf, name, 256);
        wcstombs(categoryBuf, category, 256);
        wcstombs(ownerBuf, owner, 256);
        wcstombs(statusBuf, status, 256);
        wcstombs(dateBuf, createdDate, 64);

        file << "\"" << nameBuf << "\","
            << size << ","
            << "\"" << categoryBuf << "\","
            << "\"" << ownerBuf << "\","
            << "\"" << statusBuf << "\","
            << "\"" << dateBuf << "\"\n";
        count++;
    }

    file.close();
    SQLFreeStmt(stmt, SQL_CLOSE);

    setColor(COLOR_GREEN);
    cout << "Экспорт выполнен! Сохранено " << count << " записей в файл " << filename << "\n";
    setColor(COLOR_DEFAULT);

    SQLWCHAR queryLog[] = L"INSERT INTO Logs (Action) VALUES (?)";
    SQLPrepareW(stmt, queryLog, SQL_NTS);
    wstring logMsg = L"Экспорт в CSV: " + wstring(filename.begin(), filename.end());
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, (SQLWCHAR*)logMsg.c_str(), 0, NULL);
    SQLExecute(stmt);
    SQLFreeStmt(stmt, SQL_CLOSE);
}

// Функция очистки старых записей (более 30 дней) с цветом
void cleanOldFiles() {
    SQLHSTMT stmt = db.getStatement();

    SQLWCHAR queryCount[] = L"SELECT COUNT(*) FROM Resources WHERE isDeleted = 1 AND CreatedDate < DATEADD(month, -1, GETDATE())";
    SQLRETURN ret = SQLExecDirectW(stmt, queryCount, SQL_NTS);

    int count = 0;
    if (ret == SQL_SUCCESS && SQLFetch(stmt) == SQL_SUCCESS) {
        SQLGetData(stmt, 1, SQL_C_LONG, &count, 0, NULL);
    }
    SQLFreeStmt(stmt, SQL_CLOSE);

    if (count == 0) {
        setColor(COLOR_YELLOW);
        cout << "Нет файлов для очистки (файлы в корзине старше 30 дней не найдены).\n";
        setColor(COLOR_DEFAULT);
        return;
    }

    setColor(COLOR_YELLOW);
    cout << "Найдено " << count << " файлов в корзине, созданных более 30 дней назад.\n";
    setColor(COLOR_DEFAULT);
    cout << "Вы уверены, что хотите удалить их безвозвратно? (y/n): ";
    char confirm;
    cin >> confirm;

    if (confirm != 'y' && confirm != 'Y') {
        cout << "Операция отменена.\n";
        return;
    }

    SQLWCHAR queryDelete[] = L"DELETE FROM Resources WHERE isDeleted = 1 AND CreatedDate < DATEADD(month, -1, GETDATE())";
    ret = SQLExecDirectW(stmt, queryDelete, SQL_NTS);

    if (ret == SQL_SUCCESS) {
        setColor(COLOR_GREEN);
        cout << "Удалено " << count << " старых файлов из корзины.\n";
        setColor(COLOR_DEFAULT);

        SQLWCHAR queryLog[] = L"INSERT INTO Logs (Action) VALUES (?)";
        SQLPrepareW(stmt, queryLog, SQL_NTS);
        wstring logMsg = L"Очистка старых записей: удалено " + to_wstring(count) + L" файлов";
        SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, (SQLWCHAR*)logMsg.c_str(), 0, NULL);
        SQLExecute(stmt);
    }
    else {
        setColor(COLOR_RED);
        cout << "Ошибка при очистке.\n";
        setColor(COLOR_DEFAULT);
    }

    SQLFreeStmt(stmt, SQL_CLOSE);
}

// Функция просмотра всех файлов с пагинацией (с цветом)
void viewAllFilesPaginated() {
    SQLHSTMT stmt = db.getStatement();

    SQLWCHAR queryCount[] = L"SELECT COUNT(*) FROM Resources";
    SQLRETURN ret = SQLExecDirectW(stmt, queryCount, SQL_NTS);

    int totalRecords = 0;
    if (ret == SQL_SUCCESS && SQLFetch(stmt) == SQL_SUCCESS) {
        SQLGetData(stmt, 1, SQL_C_LONG, &totalRecords, 0, NULL);
    }
    SQLFreeStmt(stmt, SQL_CLOSE);

    if (totalRecords == 0) {
        setColor(COLOR_YELLOW);
        cout << "Нет записей в базе данных.\n";
        setColor(COLOR_DEFAULT);
        return;
    }

    int pageSize = 10;
    int totalPages = (totalRecords + pageSize - 1) / pageSize;
    int currentPage = 1;

    while (true) {
        SQLWCHAR query[] = L"SELECT r.ResourceID, r.Name, r.Size, c.CategoryName, u.UserName, r.isDeleted "
            L"FROM Resources r "
            L"JOIN Categories c ON r.CategoryID = c.CategoryID "
            L"JOIN Users u ON r.OwnerID = u.UserID "
            L"ORDER BY r.ResourceID "
            L"OFFSET ? ROWS FETCH NEXT ? ROWS ONLY";

        SQLRETURN ret = SQLPrepareW(stmt, query, SQL_NTS);

        int offset = (currentPage - 1) * pageSize;
        SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &offset, 0, NULL);
        SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pageSize, 0, NULL);

        ret = SQLExecute(stmt);

        if (ret != SQL_SUCCESS) {
            setColor(COLOR_RED);
            cout << "Ошибка при выполнении запроса.\n";
            setColor(COLOR_DEFAULT);
            SQLFreeStmt(stmt, SQL_CLOSE);
            return;
        }

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

        ret = SQLPrepareW(stmt, query, SQL_NTS);
        SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &offset, 0, NULL);
        SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pageSize, 0, NULL);

        ret = SQLExecute(stmt);

        if (ret != SQL_SUCCESS) {
            setColor(COLOR_RED);
            cout << "Ошибка при выполнении запроса.\n";
            setColor(COLOR_DEFAULT);
            SQLFreeStmt(stmt, SQL_CLOSE);
            return;
        }

        setColor(COLOR_CYAN);
        cout << "\n";
        cout << "+" << string(maxNameLen + 2, '-') << "+" << string(15, '-') << "+" << string(maxOwnerLen + 2, '-') << "+" << string(20, '-') << "+" << string(12, '-') << "+\n";
        cout << "| " << left << setw(maxNameLen) << "Имя файла"
            << " | " << setw(13) << "Размер (байт)"
            << " | " << setw(maxOwnerLen) << "Владелец"
            << " | " << setw(18) << "Категория"
            << " | " << setw(10) << "Статус" << " |\n";
        cout << "+" << string(maxNameLen + 2, '-') << "+" << string(15, '-') << "+" << string(maxOwnerLen + 2, '-') << "+" << string(20, '-') << "+" << string(12, '-') << "+\n";
        setColor(COLOR_DEFAULT);

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

            char nameBuf[256];
            char ownerBuf[256];
            wcstombs(nameBuf, name, 256);
            wcstombs(ownerBuf, owner, 256);

            string strName(nameBuf);
            string strOwner(ownerBuf);

            if (strName.length() > maxNameLen) strName = strName.substr(0, maxNameLen - 3) + "...";
            if (strOwner.length() > maxOwnerLen) strOwner = strOwner.substr(0, maxOwnerLen - 3) + "...";

            string status = (isDeleted == 1) ? "В корзине" : "Активен";

            char categoryBuf[256];
            wcstombs(categoryBuf, category, 256);

            if (isDeleted == 1) {
                setColor(COLOR_RED);
            }
            else {
                setColor(COLOR_GREEN);
            }

            cout << "| " << left << setw(maxNameLen) << strName
                << " | " << setw(13) << size
                << " | " << setw(maxOwnerLen) << strOwner
                << " | " << setw(18) << categoryBuf
                << " | " << setw(10) << status << " |\n";
            count++;
        }

        setColor(COLOR_DEFAULT);
        cout << "+" << string(maxNameLen + 2, '-') << "+" << string(15, '-') << "+" << string(maxOwnerLen + 2, '-') << "+" << string(20, '-') << "+" << string(12, '-') << "+\n";
        setColor(COLOR_CYAN);
        cout << "Страница " << currentPage << " из " << totalPages << " (всего записей: " << totalRecords << ")\n";
        setColor(COLOR_DEFAULT);

        SQLFreeStmt(stmt, SQL_CLOSE);

        if (currentPage < totalPages) {
            cout << "n - следующая страница, p - предыдущая, q - выход: ";
            char input;
            cin >> input;

            if (input == 'n' || input == 'N') {
                currentPage++;
                continue;
            }
            else if (input == 'p' || input == 'P') {
                if (currentPage > 1) {
                    currentPage--;
                }
                continue;
            }
            else {
                break;
            }
        }
        else {
            setColor(COLOR_YELLOW);
            cout << "\nЭто последняя страница.\n";
            setColor(COLOR_DEFAULT);
            break;
        }
    }
}

// Функция проверки состояния сервера (пинг)
void pingServer() {
    SQLHSTMT stmt = db.getStatement();

    // Простой запрос для проверки соединения
    SQLWCHAR query[] = L"SELECT 1";
    SQLRETURN ret = SQLExecDirectW(stmt, query, SQL_NTS);

    if (ret == SQL_SUCCESS) {
        setColor(COLOR_GREEN);
        cout << "Сервер отвечает. Соединение активно.\n";
        setColor(COLOR_DEFAULT);

        // Логирование
        SQLWCHAR queryLog[] = L"INSERT INTO Logs (Action) VALUES (?)";
        SQLPrepareW(stmt, queryLog, SQL_NTS);
        wstring logMsg = L"Проверка соединения: успешно";
        SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, (SQLWCHAR*)logMsg.c_str(), 0, NULL);
        SQLExecute(stmt);
    }
    else {
        setColor(COLOR_RED);
        cout << "Сервер не отвечает! Соединение потеряно.\n";
        setColor(COLOR_DEFAULT);

        // Получаем диагностическую информацию
        SQLWCHAR sqlState[6], message[256];
        SQLINTEGER nativeError;
        SQLSMALLINT textLen;

        if (SQLGetDiagRecW(SQL_HANDLE_STMT, stmt, 1, sqlState, &nativeError, message, sizeof(message), &textLen) == SQL_SUCCESS) {
            char msgBuf[256];
            wcstombs(msgBuf, message, 256);
            setColor(COLOR_YELLOW);
            cout << "Диагностика: " << msgBuf << " (SQLSTATE: ";
            char stateBuf[6];
            wcstombs(stateBuf, sqlState, 6);
            cout << stateBuf << ")\n";
            setColor(COLOR_DEFAULT);
        }
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
        setColor(COLOR_RED);
        cout << "Не удалось подключиться к базе данных. Программа завершена.\n";
        setColor(COLOR_DEFAULT);
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
            setColor(COLOR_RED);
            cout << "Ошибка! Введите число.\n";
            setColor(COLOR_DEFAULT);
            continue;
        }

        switch (choice) {
        case 1:
            addFile();
            break;
        case 2:
            viewAllFiles();
            break;
        case 3:
            searchByName();
            break;
        case 4:
            softDeleteFile();
            break;
        case 5:
            restoreFile();
            break;
        case 6:
            showStatistics();
            break;
        case 7:
            cleanOldFiles();
            break;
        case 8:
            exportToCSV();
            break;
        case 9:
            showLogs();
            break;
        case 10:
            viewAllFilesPaginated();
            break;
        case 11:
            pingServer();
            break;
        case 0:
            cout << "Выход из программы...\n";
            break;
        default:
            setColor(COLOR_YELLOW);
            cout << "Функция будет реализована позже...\n";
            setColor(COLOR_DEFAULT);
        }
    } while (choice != 0);

    return 0;
}