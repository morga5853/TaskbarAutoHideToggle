#include <windows.h>
#include <iostream>
#include <stdio.h>

#include <aclapi.h>

#include <locale.h>





HHOOK hHook;
int tim = 0;

void ConvertToWideChar(const char* charString, LPWSTR wideString, int wideStringSize) {
    MultiByteToWideChar(CP_UTF8, 0, charString, -1, wideString, wideStringSize);
}




//открытие прав доступа
void SetRegistryKeyPermissions(HKEY hKeyRoot, LPCSTR subKey) {
    HKEY hKey;
    DWORD dwRes;

    // Открываем ключ реестра
    dwRes = RegOpenKeyExA(hKeyRoot, subKey, 0, WRITE_DAC | READ_CONTROL, &hKey);
    if (dwRes != ERROR_SUCCESS) {
        printf("Не удалось открыть ключ реестра: %d\n", dwRes);
        return;
    }

    // Получаем текущие права доступа
    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pACL = NULL;
    dwRes = GetSecurityInfo(hKey, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION,
        NULL, NULL, &pACL, NULL, &pSD);

    if (dwRes != ERROR_SUCCESS) {
        printf("Не удалось получить информацию о безопасности: %d\n", dwRes);
        RegCloseKey(hKey);
        return;
    }

    //определяем текущего пользователя
    char str1[256] = "1";
    //char username[256];
    // Преобразуем в wchar_t
    WCHAR wideUsername[256];
    DWORD size = sizeof(wideUsername);

    //ConvertToWideChar(str1, wideUsername, sizeof(wideUsername) / sizeof(WCHAR));

    if (GetUserName(wideUsername, &size)) {

        // Преобразуем wideUsername в str1
        int result = WideCharToMultiByte(CP_UTF8, 0, wideUsername, -1, str1, sizeof(str1), NULL, NULL);


        wprintf(L"Текущий пользователь: %s\n", wideUsername);
    }
    else {
        printf("Ошибка получения имени пользователя: %d\n", GetLastError());
    }


    // Создаем новый ACE для группы Users с полными правами
    EXPLICIT_ACCESSA ea;
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESSA));
    ea.grfAccessPermissions = KEY_ALL_ACCESS; // Полный доступ
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance = NO_INHERITANCE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea.Trustee.ptstrName = str1; // Указываем группу

    PACL pNewACL = NULL;
    dwRes = SetEntriesInAclA(1, &ea, pACL, &pNewACL);

    if (dwRes != ERROR_SUCCESS) {
        printf("Не удалось установить новые права доступа: %d\n", dwRes);
        LocalFree(pSD);
        RegCloseKey(hKey);
        return;
    }

    // Применяем новые права доступа к ключу
    dwRes = SetSecurityInfo(hKey, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION,
        NULL, NULL, pNewACL, NULL);

    if (dwRes != ERROR_SUCCESS) {
        printf("Не удалось применить новые права доступа: %d\n", dwRes);
    }
    else {
        printf("Права доступа успешно изменены.\n");
    }

    // Освобождаем ресурсы
    if (pNewACL) LocalFree(pNewACL);
    if (pSD) LocalFree(pSD);

    RegCloseKey(hKey);
}










// Функция для переключения режима автоскрытия панели задач
void ToggleTaskbarAutoHide() {


    HKEY hKey;
    DWORD dataType;
    BYTE data[256];
    DWORD dataSize = sizeof(data);




    // Открываем ключ реестра
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3"),
        0,
        KEY_READ,
        &hKey) == ERROR_SUCCESS) {

        // Читаем значение "Settings"
        if (RegQueryValueEx(hKey,
            TEXT("Settings"),
            NULL,
            &dataType,
            data,
            &dataSize) == ERROR_SUCCESS) {


            // Проверяем тип данных и выводим значение
            if (dataType == REG_BINARY) {
                printf("Значение 'Settings': ");
                for (DWORD i = 0; i < dataSize; i++) {
                    printf("%02X ", data[i]);
                }
                printf("\n");

                tim++;
                // Устанавливаем новое значение
                //RegSetValueExW(hKey, L"Settings", 0, REG_BINARY, (LPBYTE)&value, sizeof(value));
                if (tim == 1) {
                    if (data[8] == 2) {
                        data[8] = 03;
                    }
                    else {
                        data[8] = 02;
                    }
                }
                if (tim > 1) {
                    tim = 0;
                }
                else {
                    std::wcerr << L"NastroykaBara-" << data[8] << std::endl;


                    // Изменяем значение "Settings"
                    if (RegSetValueExW(hKey,
                        TEXT("Settings"),
                        0,
                        REG_BINARY,
                        data,
                        dataSize) == ERROR_SUCCESS) {
                        printf("Значение 'Settings' успешно изменено.\n");
                    }
                    else {
                        printf("Не удалось изменить значение 'Settings'.\n");
                    }

                    // Перезапускаем проводник для применения изменений
                    system("taskkill /f /im explorer.exe");
                    system("start explorer.exe");
                }
                printf("TIM %d\n", tim);

            }
            else {
                printf("Тип данных не соответствует ожидаемому (REG_BINARY).\n");
            }
        }
        else {
            printf("Не удалось прочитать значение 'Settings'.\n");
        }

        // Закрываем ключ
        RegCloseKey(hKey);
    }
    else {
        printf("Не удалось открыть ключ реестра.\n");
    }





}

// Функция перехвата клавиш
LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;

        // Проверяем нажатие Ctrl + K (VK_CONTROL и VK_K)
        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && pKeyBoard->vkCode == 'K') {
            ToggleTaskbarAutoHide();
            return 1; // Блокируем дальнейшую обработку
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

int main() {

    // Установка локализации
    setlocale(LC_ALL, "Russian");

    // Настройка кодировки консоли
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    //попутка открыть доступ к ключу реестра
    SetRegistryKeyPermissions(HKEY_CURRENT_USER,
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3");


    // Устанавливаем глобальный перехват клавиш
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, NULL, 0);

    if (!hHook) {
        std::cerr << "Не удалось установить хук клавиатуры." << std::endl;
        return 1;
    }

    // Цикл обработки сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        //printf("aaaa");

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook); // Снимаем хук перед выходом
    return 0;
}