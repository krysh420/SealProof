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
#define CONFIG_SRC     "config.txt" // This code should be executed in the same folder as project/updated config.txt
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

vector<string> getFilesFromConfig() { // Creates hashes for files added to config file and store them, creates backup too
    vector<string> files; 
    ifstream cfg(CONFIG_DEST);
    string line;
    while (getline(cfg, line)) {
        DWORD attr = GetFileAttributesA(line.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) { // If file not already part of it, push it to list of files
            files.push_back(line);
        } else { // If folder, iterate through folder and add files
            WIN32_FIND_DATAA fd;
            HANDLE hFind = FindFirstFileA((line + "\\*").c_str(), &fd);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                        files.push_back(line + "\\" + fd.cFileName);
                } while (FindNextFileA(hFind, &fd));
                FindClose(hFind);
            }
        }
    }
    return files; // Return vector of string containing file names
}

void backupFile(const string& filePath) { // Creates backup of files in config file
    string fileName = filePath.substr(filePath.find_last_of("\\/") + 1);
    string backupPath = string(BACKUP_FOLDER) + "\\" + fileName;

    ifstream src(filePath.c_str(), ios::binary);
    ofstream dst(backupPath.c_str(), ios::binary);
    dst << src.rdbuf();
}

int main() {
    // Step 1: Copy updated config.txt to ProgramData
    ifstream src(CONFIG_SRC, ios::binary);
    ofstream dst(CONFIG_DEST, ios::binary);
    if (!src || !dst) {
        MessageBoxA(NULL, "Failed to update config.txt", "SealProof", MB_ICONERROR);
        return 1;
    }
    dst << src.rdbuf();

    // Step 2: Create backup folder
    createDirectory(BACKUP_FOLDER);

    // Step 3: Generate and save hashes, create backups
    ofstream out(HASH_PATH, ios::trunc);
    vector<string> files = getFilesFromConfig();

    for (const string& file : files) {
        string hash = getFileHash(file);
        if (!hash.empty()) {
            out << file << "|" << hash << endl;
            backupFile(file);
        }
    }

    MessageBoxA(NULL, "Config updated and hashes regenerated.", "SealProof", MB_OK | MB_ICONINFORMATION);
    return 0;
}








// PS: Remember to enjoy the light, before it's gone  