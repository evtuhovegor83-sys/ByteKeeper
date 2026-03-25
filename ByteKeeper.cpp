#include <iostream>
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
        case 0:
            cout << "Выход из программы...\n";
            break;
        default:
            cout << "Функция будет реализована позже...\n";
        }
    } while (choice != 0);

    return 0;
}