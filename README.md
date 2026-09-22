# Enterprise-File-Integrity-Monitor (FIM)

A lightweight, native C++ tool designed for batch integrity auditing of enterprise SAN storage. It performs deep cryptographic verification (SHA-256) between Live environments and Cold Backup drives, identifying altered, missing, or newly introduced files without relying on external dependencies or runtime environments.

Built in standard C++17. Unlike Java or Python-based alternatives, it compiles into a standalone `.exe` that runs natively on any Windows machine/server without requiring a JRE or specific frameworks.
Implements the native Windows Cryptographic API (`wincrypt.h` / `Advapi32.dll`) to compute SHA-256 hashes at low level, maximizing execution speed.
Specifically designed to handle massive enterprise files (e.g., large database dumps or heavy PDF archives) via an 8 kB static buffer processing.

INSTRUCTIONS:

1) Create a text file and call it FIM_config.txt. Inside, you must define your base directories using BACKUP= and LIVE=. Then, type the specific sub-directories you want to compare. You can also type comments by starting a line with the # symbol. I provided an example in this repo

2) Create a folder and call it as you please. IMPORTANT: FIM_config.txt and FIM.exe both need to stay in this folder, otherwise it won't work
 
3) As soon as the process is over, a Report_FIM.txt file will be generated in the folder (or overwritten if already existing). It will contain the detailed output of the analysis, including execution time and the size of the scanned directories
