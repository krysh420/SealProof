// ========================
// update_config.cpp 
// ========================
// Not the best at commenting, forgive me

// Forgot to mention in later code, but this also copies config from project folder to folder where SealProof is already setup
// Import libraries
#include <windows.h>
#include <wincrypt.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <shlobj.h>
#include <direct.h>
#include <io.h>

using namespace std;

// Directories used NOTE: BE SURE IT IS SAME FOR BOTH monitor.cpp AND THIS FILE
#define HASH_PATH      "C:\\ProgramData\\SealProof\\stored_hashes.txt"
#define CONFIG_SRC     "config.txt"
#define CONFIG_DEST    "C:\\ProgramData\\SealProof\\config.txt"
#define BACKUP_FOLDER  "C:\\ProgramData\\SealProof\\backups"

#ifndef CALG_SHA_256 // Define that we use SHA_256 hashing
#define CALG_SHA_256 0x0000800c
#endif

string getFileHash(const string& path) { // Get file hash and store in hex string
    ifstream file(path.c_str(), ios::binary);
    if (!file.is_open()) return "";

    HCRYPTPROV hProv; // Handle to crypto provider
    HCRYPTHASH hHash; // Handle to hash object
    BYTE hash[32]; // 32 Byte array for resulting hash. SHA-256 = 256 bits = 32 bytes
    DWORD hashLen = sizeof(hash);
    stringstream ss;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return "";  // Initialize crypto provider context, if fail return string
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) { CryptReleaseContext(hProv, 0); return ""; } // Create has object, if fail, return empty string 

    char buf[4096];
    while (file.read(buf, sizeof(buf))) CryptHashData(hHash, (BYTE*)buf, file.gcount(), 0); // Feed data to hash function 4KB at a time since feeding all once maybe not feasible
    if (file.gcount() > 0) CryptHashData(hHash, (BYTE*)buf, file.gcount(), 0);//               for files which are greater than size of RAM on the system.

    if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
        for (DWORD i = 0; i < hashLen; ++i)
            ss << hex << setw(2) << setfill('0') << (int)hash[i]; // Finalise and convert hash to hex string
    }

    CryptDestroyHash(hHash); // Free crypto stuff and release memory
    CryptReleaseContext(hProv, 0);
    return ss.str(); // Return hash string
}

bool createDirectory(const char* path) { // Creates directory if not exist
    if (_access(path, 0) == 0) return true;
    return _mkdir(path) == 0;
}


vector<string> getFilesFromConfig() {
    vector<string> files;
    ifstream cfg(CONFIG_DEST);
    if (!cfg.is_open()) {
        MessageBoxA(NULL, "Failed to open ProgramData\\config.txt", "SealProof", MB_ICONERROR);
        return files;
    }

    string line;
    while (getline(cfg, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty()) continue;

        DWORD attr = GetFileAttributesA(line.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            files.push_back(line);
        } else if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
            WIN32_FIND_DATAA fd;
            HANDLE hFind = FindFirstFileA((line + "\\*").c_str(), &fd);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                        files.push_back(line + "\\" + fd.cFileName);
                } while (FindNextFileA(hFind, &fd));
                FindClose(hFind);
            }
        } else {
            // Debug: Warn invalid entry
            // string msg = "Invalid or unreadable path: " + line;
            // MessageBoxA(NULL, msg.c_str(), "SealProof", MB_ICONWARNING);
        }
    }

    return files;
}
void backupFile(const string& filePath) { // Creates backup of files in config file
    string fileName = filePath.substr(filePath.find_last_of("\\/") + 1);
    string backupPath = string(BACKUP_FOLDER) + "\\" + fileName;

    ifstream src(filePath.c_str(), ios::binary);
    ofstream dst(backupPath.c_str(), ios::binary);
    dst << src.rdbuf();
}


int main() {
    // === Step 1: Copy updated config.txt to ProgramData
    ifstream src(CONFIG_SRC, ios::binary);
    if (!src.is_open()) {
        MessageBoxA(NULL, "Could not find config.txt in current folder.", "SealProof", MB_ICONERROR);
        return 1;
    }

    ofstream dst(CONFIG_DEST, ios::binary);
    if (!dst.is_open()) {
        MessageBoxA(NULL, "Could not write to ProgramData\\config.txt", "SealProof", MB_ICONERROR);
        return 1;
    }

    dst << src.rdbuf();
    src.close();
    dst.close();

    // === Step 2: Create backup folder
    createDirectory(BACKUP_FOLDER);

    // === Step 3: Read config and generate hashes
    vector<string> files = getFilesFromConfig();
    if (files.empty()) {
        MessageBoxA(NULL, "No valid files found in config.txt", "SealProof", MB_ICONERROR);
        return 1;
    }

    string tempHashPath = string(HASH_PATH) + ".tmp";
    ofstream out(tempHashPath, ios::trunc);
    if (!out.is_open()) {
        MessageBoxA(NULL, "Failed to create temporary hash file.", "SealProof", MB_ICONERROR);
        return 1;
    }

    bool wroteSomething = false;
    for (const string& file : files) {
        string hash = getFileHash(file);
        if (!hash.empty()) {
            out << file << "|" << hash << endl;
            backupFile(file);
            wroteSomething = true;
        }
    }

    out.close();

    if (wroteSomething) {
        remove(HASH_PATH);
        rename(tempHashPath.c_str(), HASH_PATH);
        MessageBoxA(NULL, "Config updated and hashes regenerated.", "SealProof", MB_OK | MB_ICONINFORMATION);
    } else {
        remove(tempHashPath.c_str());
        MessageBoxA(NULL, "No valid hashes were generated. Check file paths.", "SealProof", MB_ICONWARNING);
    }

    return 0;
}







// PS: Remember to enjoy the light, before it's gone  
