# Enterprise-File-Integrity-Monitor
A lightweight, native C++ tool designed for batch integrity auditing of enterprise SAN storage. It performs deep cryptographic verification (SHA-256) between Live environments and Cold Backup drives, identifying altered, missing, or newly introduced files without relying on external dependencies or runtime environments.

Built in standard C++17. Unlike Java or Python-based alternatives, it compiles into a standalone `.exe` that runs natively on any Windows machine/server without requiring a JRE or specific frameworks.
Implements the native Windows Cryptographic API (`wincrypt.h` / `Advapi32.dll`) to compute SHA-256 hashes at low level, maximizing execution speed.
Specifically designed to handle massive enterprise files (e.g., large database dumps or heavy PDF archives) via an 8 kB static buffer processing.

INSTRUCTIONS:
Aprire il cmd ed eseguire cd C:\Users\*nome_utente*\Desktop per spostare il terminale nel desktop
Open the cmd and execute cd C:\Users\*username*\Desktop to move the terminal to the desktop
Then double click on the exe or alternatively execute from the cmd .\FIM.exe
As soon as the process is over, the Report_FIM.txt file will be generated on the desktop or overwritten if already existing. It will contain the detailed output of the analysis
