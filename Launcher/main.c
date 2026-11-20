/*
Copywrite (C) stupid idiot known as alen or Astro on discord.
i will hunt you down if you steal my code if you do not credit me
im super duper evil >:) totaly totaly lol

Names of programers contrubuting to this file:
- Alen

*/


#include <stdio.h>
#include <string.h>
#include <ctype.h>
// i run linux so ofc theres gona be a linux port!
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif
// these are literaly self explanitory
#define BASE_URL    "http://localhost"
#define CLIENT_EXE  "Roblox.exe"
#define PREFIX      "vetrablox:join:"

static int fail(const char *msg) {
#ifdef _WIN32
    MessageBoxA(NULL, msg, "Vetrablox", MB_OK | MB_ICONERROR);
#else
    fprintf(stderr, "Vetrablox: %s\n", msg);
#endif
    return 1;
}

static int all_d(const char *s) {
    for (; *s; s++)
        if (!isdigit((unsigned char)*s))
            return 0;
    return 1;
}

// pulls place id
static int parse_place_id(const char *uri, char *out, size_t out_size) {
    size_t plen = strlen(PREFIX);
    if (strncmp(uri, PREFIX, plen) != 0)
        return 0;
    const char *id = uri + plen;
    size_t len = strlen(id);
    if (len > 0 && id[len - 1] == '/')
        len--;
    if (len == 0 || len >= out_size)
        return 0;
    memcpy(out, id, len);
    out[len] = '\0';
    return all_d(out);
}

static int get_ldir(char *dir, size_t size) {
#ifdef _WIN32
    DWORD n = GetModuleFileNameA(NULL, dir, (DWORD)size);
    if (n == 0 || n >= size) return 0;
    char *slash = strrchr(dir, '\\');
#else
    ssize_t n = readlink("/proc/self/exe", dir, size - 1);
    if (n <= 0) return 0;
    dir[n] = '\0';
    char *slash = strrchr(dir, '/');
#endif
    if (slash) *slash = '\0';
    return 1;
}

int main(int argc, char **argv) {
    // argv is the link that was clicked
    if (argc < 2)
        return fail("Launch a game from the site!");
    char place_id[16];
    if (!parse_place_id(argv[1], place_id, sizeof place_id))
        return fail("Bad launch link.");
    char dir[4096];
    if (!get_ldir(dir, sizeof dir))
        return fail("Couldnt find the folder that the launcher is in, how does this happen?");
    char script[256];
    snprintf(script, sizeof script, "loadfile('%s/game/join.ashx?placeId=%s')()", BASE_URL, place_id);

#ifdef _WIN32
    char cmd[4096 + 512];
    snprintf(cmd, sizeof cmd, "\"%s\\%s\" -script \"%s\"", dir, CLIENT_EXE, script);

    STARTUPINFOA si = { sizeof si };
    PROCESS_INFORMATION pi; // "guys lets process the entirety of pi"
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, dir, &si, &pi))
        return fail("were the hell did you put your roblox?");
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
#else  
    if (chdir(dir) != 0)
        return fail("Couldnt open were the launcher is located");
    char *args[] = { "wine", CLIENT_EXE, "-script", script, NULL };
    execvp("wine", args);

    return fail("Couldnt start wine, is it even on this linux pc? i hope it is... look online for how to install it for your distrubution of linux");
#endif
}