#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <map>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <windows.h>
#include <wincrypt.h>

namespace fs = std::filesystem;

std::ofstream fileReport;

void logMsg(const std::string& messaggio)
{
    std::cout << messaggio << std::endl;
    if(fileReport.is_open())
    {
        fileReport << messaggio << std::endl;
    }
}

double calcolaDimensioneMB(const std::string& rootPathStr)
{
    double totalSizeBytes = 0.0;
    try
    {
        for(const auto& entry : fs::recursive_directory_iterator(rootPathStr))
        {
            if(fs::is_regular_file(entry))
            {
                totalSizeBytes += fs::file_size(entry);
            }
        }
    }
    catch(const fs::filesystem_error& e){}
    return totalSizeBytes / (1024.0 * 1024.0); // Converte da byte a Megabyte
}

std::string calcolaHash(const std::string& filePath)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::string hashStr = "";

    if(!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return "";
    if(!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
    {
        CryptReleaseContext(hProv, 0);
        return "";
    }

    std::ifstream file(filePath, std::ios::binary);
    if(!file)
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }

    const int bufSize = 8192;
    std::vector<char> buffer(bufSize);
    
    while(file.read(buffer.data(), bufSize) || file.gcount() > 0)
    {
        CryptHashData(hHash, reinterpret_cast<const BYTE*>(buffer.data()), file.gcount(), 0);
    }
    file.close();

    DWORD hashLen = 0;
    DWORD count = sizeof(DWORD);
    CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashLen, &count, 0);

    std::vector<BYTE> hashValue(hashLen);
    if(CryptGetHashParam(hHash, HP_HASHVAL, hashValue.data(), &hashLen, 0))
    {
        std::stringstream ss;
        for(DWORD i=0;i<hashLen;i++)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hashValue[i];
        }
        hashStr = ss.str();
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return hashStr;
}

std::map<std::string, std::string> scansionaDirectory(const std::string& rootPathStr)
{
    std::map<std::string, std::string> hashDatabase;
    fs::path rootPath(rootPathStr);

    try
    {
        for(const auto& entry : fs::recursive_directory_iterator(rootPath))
        {
            if(fs::is_regular_file(entry))
            {
                std::string percorsoRelativo = fs::relative(entry.path(), rootPath).string();
                std::string hash = calcolaHash(entry.path().string());
                
                if(!hash.empty())
                {
                    hashDatabase[percorsoRelativo] = hash;
                }
                else
                {
                    logMsg("    [!] Errore di lettura: " + entry.path().string());
                }
            }
        }
    }
    catch(const fs::filesystem_error& e)
    {
        logMsg("    [!] Errore di sistema: " + std::string(e.what()));
    }

    return hashDatabase;
}

bool confrontaSnapshot(const std::map<std::string, std::string>& backup, const std::map<std::string, std::string>& live)
{
    bool anomalieTrovate = false;

    for(const auto& [percorsoRelativo, hashBackup] : backup)
    {
        auto it = live.find(percorsoRelativo);
        if(it == live.end())
        {
            logMsg("    [MANCANTE] Nel Backup ma cancellato nel Live: " + percorsoRelativo);
            anomalieTrovate = true;
        }
        else
        {
            if(hashBackup != it->second)
            {
                logMsg("    [ALTERATO] File modificato nel Live: " + percorsoRelativo);
                anomalieTrovate = true;
            }
        }
    }

    for(const auto& [percorsoRelativo, hashLive] : live)
    {
        if(backup.find(percorsoRelativo) == backup.end())
        {
            logMsg("    [NUOVO] Trovato file nel Live non tracciato nel Backup: " + percorsoRelativo);
            anomalieTrovate = true;
        }
    }

    return anomalieTrovate;
}

int main()
{
    auto start_time = std::chrono::high_resolution_clock::now();

    // Recupera il desktop in base all'utente Windows corrente
    const char* userProfilePath = std::getenv("USERPROFILE");
    if(userProfilePath == nullptr)
    {
        std::cerr << "[ERRORE CRITICO] Impossibile determinare la cartella utente di Windows." << std::endl;
        system("pause");
        return 1;
    }

    std::string desktopPath = std::string(userProfilePath) + "\\Desktop";
    std::string configPath = desktopPath + "\\FIM_config.txt";
    std::string reportPath = desktopPath + "\\Report_FIM.txt";

    fileReport.open(reportPath, std::ios::trunc);

    logMsg("==================================================");
    logMsg("INIZIO SCANSIONE DI INTEGRITA' MASSIVA (C++ NATIVE)");
    logMsg("==================================================\n");

    std::ifstream fileConfig(configPath);
    if(!fileConfig.is_open())
    {
        logMsg("[ERRORE CRITICO] File di configurazione non trovato: " + configPath);
        if (fileReport.is_open()) fileReport.close();
        system("pause");
        return 1;
    }

    std::string baseBackup = "";
    std::string baseLive = "";
    std::vector<std::string> percorsiDaControllare;
    std::string linea;

    while(std::getline(fileConfig, linea))
    {
        if(linea.empty() || linea[0] == '#') continue;

        if(linea.find("BACKUP=") == 0)
        {
            baseBackup = linea.substr(7);
            if(!baseBackup.empty() && baseBackup.back() != '\\' && baseBackup.back() != '/')
            {
                baseBackup += "\\";
            }
        } 
        else if(linea.find("LIVE=") == 0)
        {
            baseLive = linea.substr(5);
            if(!baseLive.empty() && baseLive.back() != '\\' && baseLive.back() != '/')
            {
                baseLive += "\\";
            }
        } 
        else
        {
            percorsiDaControllare.push_back(linea);
        }
    }
    fileConfig.close();

    if(baseBackup.empty() || baseLive.empty())
    {
        logMsg("[ERRORE CRITICO] Radici BACKUP= o LIVE= mancanti nel file di configurazione.");
        if(fileReport.is_open()) fileReport.close();
        system("pause");
        return 1;
    }

    for(const auto& subPath : percorsiDaControllare)
    {
        std::string pathBackup = baseBackup + subPath;
        std::string pathLive = baseLive + subPath;

        double sizeMB = calcolaDimensioneMB(pathBackup);
        
        std::stringstream ssSize;
        ssSize << std::fixed << std::setprecision(2) << sizeMB;

        logMsg(">>> Analisi in corso su: " + subPath + " (" + ssSize.str() + " MB)\n");

        if(!fs::exists(pathBackup) || !fs::exists(pathLive))
        {
            logMsg("  [SKIPPED] Cartelle non trovate su uno o entrambi i dischi (" + pathBackup + " / " + pathLive + ")");
            logMsg("--------------------------------------------------\n");
            continue;
        }

        std::map<std::string, std::string> snapshotBackup = scansionaDirectory(pathBackup);
        std::map<std::string, std::string> snapshotLive = scansionaDirectory(pathLive);

        bool anomalie = confrontaSnapshot(snapshotBackup, snapshotLive);

        if(anomalie)
        {
            logMsg("\n  [ALLARME] Il disco LIVE NON corrisponde al BACKUP per la cartella " + subPath);
        }
        else
        {
            logMsg("\n  Dati per la cartella " + subPath + " OK");
        }
        logMsg("--------------------------------------------------\n");
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;

    std::stringstream ssFinal;
    ssFinal << "==================================================\n";
    ssFinal << "SCANSIONE MASSIVA COMPLETATA. (ANALISI DURATA " 
            << std::fixed << std::setprecision(2) << duration.count() << " SECONDI)";
    
    logMsg(ssFinal.str());
    
    if(fileReport.is_open()) fileReport.close();
    
    system("pause");
    return 0;
}