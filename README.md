# 🛡️ SealProof

**SealProof** is a lightweight, forensic-grade file integrity monitoring tool for Windows systems.  
Designed for digital forensics, evidence preservation, or tamper detection during sensitive operations, it ensures that any change to critical files or folders is **detected, logged, and reported** — even across system reboots.

---

## 🔍 Features

| Feature                    | Description                                                                 |
|----------------------------|-----------------------------------------------------------------------------|
| 🗂️ File + Folder Monitoring | Supports both individual files and entire directories (recursively)         |
| 🔐 Tamper Alerts           | Detects unauthorized changes and alerts the user                            |
| 📜 Logging                 | Detailed logs written to `integrity_log.txt` with timestamps                 |
| 🔄 Update Config Utility   | `update_config.exe` lets you safely modify the list of monitored targets     |
| 🧠 Startup Support         | Easily configure the app to run silently at system startup                   |
| 📁 Minimal Footprint       | Built in C++, runs from system tray with minimal CPU and memory usage        |

---

## ⚙️ Setup Instructions

### 🔧 Requirements

* ✅ Windows 10 or 11
* ✅ C++17-capable compiler (e.g. MinGW-w64)
* ✅ Admin privileges (if configuring autostart via registry)

---

### 🛠 1. Build the Project

You can build everything using the provided batch file:

```bat
setup.bat
```

> It will generate `monitor.exe` and `update_config.exe` in the current directory.

---

### 📝 2. Configure Monitored Paths

Edit `config.txt` with absolute paths to the files/folders you want to protect. Example:

```
C:\Users\YourName\Documents\important.txt
D:\Evidence\CaseA\
```

✅ Use full absolute paths
✅ Directories are scanned **recursively**
✅ See `config_instructions.txt` for help

---

### 🧪 3. First Run: Generate Hashes

```bash
monitor.exe
```

This will:

* Hash all listed files
* Create `stored_hashes.txt` with baseline hashes
* Make it read-only to prevent tampering

---

### 🔄 4. Add to Startup (Optional)

To run `monitor.exe` on every system boot:

**Method 1: Manual Startup Folder**

* Press `Win + R`, type `shell:startup`, and press Enter
* Place a shortcut to `monitor.exe` in that folder

**Method 2: Modify Registry (Advanced)**
Update `setup.bat` or use `reg add` in PowerShell:

```powershell
reg add HKCU\Software\Microsoft\Windows\CurrentVersion\Run /v SealProof /t REG_SZ /d "C:\Path\To\monitor.exe"
```

---

### ✏️ 5. Update Config via CLI

* Add entry to config.txt in project folder.
* Run `update_config.exe`
```bash
update_config.exe
```

---

## 🚨 Tamper Detection

* On every run, `monitor.exe` compares current hashes with `stored_hashes.txt`
* If any mismatch is found:

  * A popup is shown
  * Details are logged to `integrity_log.txt`
* On shutdown or relaunch, updated hashes are stored again (if configured)

---

## 📜 License

Licensed under the [Apache License 2.0](./LICENSE.md).
You are free to use, modify, and distribute this project with proper attribution.

---

## 👤 Author

**Krish Mishra**
🔗 [LinkedIn](https://www.linkedin.com/in/krish-mishra-a9410917b/)
🔗 [GitHub](https://github.com/krysh420)

---

## 🧠 Future Plans

* [ ] Exportable logs
* [ ] Email alerts on detection and cloud backups
* [ ] System hardening (WMI detection, anti-debug, etc.)

---

## 🆘 Support

If you encounter bugs, need a feature, or want to contribute:

* Open an issue
* Submit a pull request
* Or contact [Krish on GitHub](https://github.com/krysh420)
