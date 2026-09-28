/*
 * explore.exe - start Explore: the Perl beside this program (perl\bin\perl.exe)
 * runs explore.pl, which runs the game's BASIC on the MBasic interpreter.
 *
 * The command line is passed on unchanged, so explore.exe takes exactly the
 * options explore.pl takes (explore.exe --help).  The console and the standard
 * handles are shared with Perl; explore.exe waits for it and returns its exit
 * code.
 *
 * Perl's own environment variables are cleared for the child, so that a Perl
 * installed on the computer cannot lend it modules of another version.
 */
#include <windows.h>

/* Ctrl+C and Ctrl+Break go to both processes: the game decides what they do
   (it ends, or ignores them in ^quit mode), and this one keeps waiting for it.
   A handler routine is not inherited, unlike SetConsoleCtrlHandler(NULL, TRUE),
   which would switch Ctrl+C off in the game too. */
static BOOL WINAPI on_ctrl(DWORD type)
{
    return type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT;
}

/* the arguments: the command line after the program name, which is parsed as
   the C runtime parses argv[0] (quoted, or up to the first blank) */
static const wchar_t *arguments(const wchar_t *cl)
{
    if (*cl == L'"') {
        for (cl++; *cl && *cl != L'"'; cl++)
            ;
        if (*cl)
            cl++;
    } else {
        while (*cl && *cl != L' ' && *cl != L'\t')
            cl++;
    }
    while (*cl == L' ' || *cl == L'\t')
        cl++;
    return cl;
}

static void fail(const wchar_t *what, const wchar_t *path)
{
    DWORD err = GetLastError(), n;
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    wchar_t msg[700];
    char out[1400];
    int len;

    wsprintfW(msg, L"explore: %s %s (error %lu)\r\n", what, path, err);
    if (!WriteConsoleW(h, msg, lstrlenW(msg), &n, NULL)) {
        len = WideCharToMultiByte(CP_ACP, 0, msg, -1, out, sizeof out, NULL, NULL);
        if (len > 1)
            WriteFile(h, out, len - 1, &n, NULL);
    }
}

int wmain(void)
{
    static wchar_t dir[MAX_PATH], perl[MAX_PATH + 32], script[MAX_PATH + 32];
    static const wchar_t *const cleared[] = {
        L"PERL5LIB", L"PERLLIB", L"PERL5OPT", L"PERL5DB", L"PERLIO",
        L"PERL_UNICODE", L"PERL_HASH_SEED", L"PERL5SHELL",
    };
    const wchar_t *args;
    wchar_t *cmd, *p;
    DWORD n, code = 1;
    unsigned i;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    n = GetModuleFileNameW(NULL, dir, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        fail(L"cannot find its own folder", L"");
        return 1;
    }
    p = dir + n;
    while (p > dir && p[-1] != L'\\' && p[-1] != L'/')
        p--;
    *p = 0;

    lstrcpyW(perl, dir);
    lstrcatW(perl, L"perl\\bin\\perl.exe");
    lstrcpyW(script, dir);
    lstrcatW(script, L"explore.pl");
    if (GetFileAttributesW(perl) == INVALID_FILE_ATTRIBUTES) {
        fail(L"missing", perl);
        return 1;
    }
    if (GetFileAttributesW(script) == INVALID_FILE_ATTRIBUTES) {
        fail(L"missing", script);
        return 1;
    }

    /* "perl.exe" "explore.pl" <the arguments as given> */
    args = arguments(GetCommandLineW());
    cmd = HeapAlloc(GetProcessHeap(), 0,
                    (lstrlenW(perl) + lstrlenW(script) + lstrlenW(args) + 8) * sizeof(wchar_t));
    if (!cmd)
        return 1;
    lstrcpyW(cmd, L"\"");
    lstrcatW(cmd, perl);
    lstrcatW(cmd, L"\" \"");
    lstrcatW(cmd, script);
    lstrcatW(cmd, L"\" ");
    lstrcatW(cmd, args);

    for (i = 0; i < sizeof cleared / sizeof cleared[0]; i++)
        SetEnvironmentVariableW(cleared[i], NULL);

    SetConsoleCtrlHandler(on_ctrl, TRUE);

    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    if (!CreateProcessW(perl, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        fail(L"cannot start", perl);
        return 1;
    }
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    return (int)code;
}
