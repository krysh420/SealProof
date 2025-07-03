// ========================
// monitor.cpp
// ========================
// Not the best at commenting, forgive me

#ifndef CALG_SHA_256 // Defines use of SHA 256 
#define CALG_SHA_256 0x0000800c
#endif

// Importing Libraries
#include <windows.h>
#include <wincrypt.h>
#include <shlwapi.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <iomanip>
#include <direct.h>

using namespace std;

// Directories used
const string HASH_FILE = "C:\\ProgramData\\SealProof\\stored_hashes.txt"; // Feel free to tweak these as per your liking, you can keep them in another folder
const string CONFIG_FILE = "C:\\ProgramData\\SealProof\\config.txt"; //      since you may want to hide it
const string LOG_FILE = "C:\\ProgramData\\SealProof\\integrity_log.txt";
const string BACKUP_DIR = "C:\\ProgramData\\SealProof\\backups\\";

// Currently the time has to be updated manually and the code needs to be recompiled each time. In future update I would like to change that

// Time after which user will not use system:
const int LAST_USAGE_HOUR = 23;  // 11 PM
const int LAST_USAGE_MINUTE = 30;
// Time after which user will use system the next day:
const int FIRST_USAGE_HOUR = 7;
const int FIRST_USAGE_MINUTE = 0;


string sanitizeFilename(const string& path) {
    string safe = path; // Sanitize file names so they dont interfere when creating backups (Windows does not like use of slashes and colons in names)
    for (char& c : safe) {
        if (c == ':' || c == '\\' || c == '/') c = '_';
    }
    return safe;
}

bool isInRestrictedTime(time_t t) { // As the name suggests
    tm* local = localtime(&t);
    int hour = local->tm_hour;
    int minute = local->tm_min;
    return (hour >= LAST_USAGE_HOUR && minute >= LAST_USAGE_MINUTE) || (FIRST_USAGE_MINUTE >= 0 && hour <= FIRST_USAGE_HOUR);
}

string getFileHash(const string& path) {
    ifstream file(path, ios::binary); // Create SHA-256 hashes of file in config file
    if (!file.is_open()) return "";

    HCRYPTPROV hProv; // Cryptographic context provider https://learn.microsoft.com/en-us/windows/win32/seccrypto/hcryptprov
    HCRYPTHASH hHash; // https://learn.microsoft.com/en-us/windows/win32/seccrypto/hcrypthash
    BYTE hash[32]; // 32 Byte array for resulting hash. SHA-256 = 256 bits = 32 bytes
    DWORD hashLen = sizeof(hash); // Holds size of hash
    stringstream ss; // Stores final hash string

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return ""; // Initialize cryptographic context, if fails: return 
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) { CryptReleaseContext(hProv, 0); return ""; } // Create cryptographic hash, if fails: release crypto provider and exits

    char buf[4096]; // Buffer: stores file chunks in 4KB
    while (file.read(buf, sizeof(buf))) CryptHashData(hHash, (BYTE*)buf, file.gcount(), 0); // Files are passed in 4KB chunks in hash function since big files cannot be passed at once
    if (file.gcount() > 0) CryptHashData(hHash, (BYTE*)buf, file.gcount(), 0); // file.read() returns false on last incomplete chunk (size of last buf is less than 4KB since it's not necessary files are multiple of 4KB)
    //                                                                            so hash for the incomplete chunks created seperated 

    if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) { // Retrieve final hash and convert it to readable hexa string
        for (DWORD i = 0; i < hashLen; ++i)
            ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }

    CryptDestroyHash(hHash); // Free memory
    CryptReleaseContext(hProv, 0); 
    return ss.str(); // Returns final hash string
}

vector<string> getFiles() {
    vector<string> files; // List of all files to be considered
    ifstream cfg(CONFIG_FILE); // Read config
    string line; 
    while (getline(cfg, line)) {
        DWORD attr = GetFileAttributesA(line.c_str()); // Checks whether path is file or folder
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            files.push_back(line); // Add to list if valid file
        } else {
            WIN32_FIND_DATAA fd; 
            HANDLE hFind = FindFirstFileA((line + "\\*").c_str(), &fd); // Incase of folder, try to get first file in the folder
            if (hFind != INVALID_HANDLE_VALUE) { // If valid file detected
                do {
                    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
                        files.push_back(line + "\\" + fd.cFileName); // Add it to file list
                } while (FindNextFileA(hFind, &fd)); // Loop and find next file
                FindClose(hFind); 
            }
        }
    }
    return files; // Return list(vector) of files
}

void logTampering(const string& message) { // This function writes a message into a log file (integrity_log.txt) with a timestamp,
    ofstream log(LOG_FILE, ios::app); //      so that you can keep track of when file tampering was detected and what files were involved
    time_t now = time(0);
    log << "[" << ctime(&now) << "] " << message << endl;
}

time_t getFileAccessTime(const string& path) { // Retrieve last access time of file
    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL); 
    if (hFile == INVALID_HANDLE_VALUE) return 0; // Return 0 if invalid file handle

    FILETIME ft; 
    GetFileTime(hFile, NULL, NULL, &ft); // Retrieves time stamps (first = creation time, second = last write time, third = last access time) first 2 null since we only want last access
    CloseHandle(hFile); 

    /*
    Windows time is in 100 nanosecs intervals since January 16 1601
    ULARGE INTEGER merges 32 bit to 64 bit parts
    */

    ULARGE_INTEGER ull; 
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    // 116444736000000000 is the number of 100-nanosecond intervals between 1601 and 1970
    // Dividing it to convert to secs
    // So we return a time_t in standard UNIX time
    return (ull.QuadPart - 116444736000000000ULL) / 10000000ULL;
}

void backupFile(const string& path) { // Creates back up file
    _mkdir(BACKUP_DIR.c_str()); // Backup directory defined above, feel free to tweak it. In future I would like to integrate cloud backups
    SetFileAttributesA(BACKUP_DIR.c_str(), FILE_ATTRIBUTE_HIDDEN); // Backup directory hidden

    time_t now = time(0); // Check current time
    tm* lt = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "__%Y%m%d_%H%M.bak", lt); 

    string backupPath = BACKUP_DIR + sanitizeFilename(path) + timestamp; // Backup file format: <DriveLetter>:_path_to_file.<extension>.bak, Simply rename to original extension to restore

    CopyFileA(path.c_str(), backupPath.c_str(), FALSE); // Save to backup directory
}

void saveHashes(const map<string, string>& hashes) { // Stores hashes in stored_hashes.txt file where it is defined
    ofstream out(HASH_FILE); 
    for (const auto& p : hashes) 
        out << p.first << "|" << p.second << endl; // Hashes stored in format: <hash>:Path/to/file/from/config.txt
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { // Main function, refer to https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-api-list
    map<string, string> stored;
    ifstream in(HASH_FILE); // Loading stored hashes
    string line;
    while (getline(in, line)) {
        size_t sep = line.find('|');
        if (sep != string::npos)
            stored[line.substr(0, sep)] = line.substr(sep + 1);
    }

    map<string, string> updated; // Check current hash file
    string tampered; // Stores path of tampered files

    for (const auto& file : getFiles()) {
        string hash = getFileHash(file);
        time_t accessTime = getFileAccessTime(file);

        if (!stored.count(file)) {
            updated[file] = hash;
            continue;
        }

        if (stored[file] != hash && isInRestrictedTime(accessTime)) {
            tampered += file + "\n";
            logTampering("Tampered File (Suspicious Time): " + file);
            backupFile(file);
        } else {
            updated[file] = hash;
            backupFile(file);
        }
    }

    saveHashes(updated);

    if (!tampered.empty())
        MessageBoxA(NULL, tampered.c_str(), "SealProof - Tampered Files", MB_ICONWARNING);

    return 0;
}








// PS: Remember to enjoy the light, before it's gone  