#include "aes_gcm.h"
#include <iostream>
#include <cstdio>
#include <vector>


void printHex(BYTE* data, size_t size ){
    for(size_t i = 0; i < size; i++){
        printf(" %02x ", data[i]);
    }

    printf("\n");
}

AESGCM:: ~AESGCM(){
    Cleanup();
}

// Freebie: initialize AES class
AESGCM::AESGCM( BYTE key[AES_256_KEY_SIZE]){
    hAlg = 0;
    hKey = NULL;

    // create a handle to an AES-GCM provider
    nStatus = ::BCryptOpenAlgorithmProvider(
        &hAlg, 
        BCRYPT_AES_ALGORITHM, 
        NULL, 
        0);
    if (! NT_SUCCESS(nStatus))
    {
        printf("**** Error 0x%x returned by BCryptOpenAlgorithmProvider\n", nStatus);
        Cleanup();
        return;
    }
    if (!hAlg){
        printf("Invalid handle!\n");
    }
    nStatus = ::BCryptSetProperty(
        hAlg, 
        BCRYPT_CHAINING_MODE, 
        (BYTE*)BCRYPT_CHAIN_MODE_GCM, 
        sizeof(BCRYPT_CHAIN_MODE_GCM), 
        0);
    if (!NT_SUCCESS(nStatus)){
         printf("**** Error 0x%x returned by BCryptGetProperty ><\n", nStatus);
         Cleanup();
         return;
    }
  

    nStatus = ::BCryptGenerateSymmetricKey(
        hAlg, 
        &hKey, 
        NULL, 
        0, 
        key, 
        AES_256_KEY_SIZE, 
        0);
    if (!NT_SUCCESS(nStatus)){
        printf("**** Error 0x%x returned by BCryptGenerateSymmetricKey\n", nStatus);
        Cleanup();
        return;
    }
    DWORD cbResult = 0;
     nStatus = ::BCryptGetProperty(
         hAlg, 
         BCRYPT_AUTH_TAG_LENGTH, 
         (BYTE*)&authTagLengths, 
         sizeof(authTagLengths), 
         &cbResult, 
         0);
   if (!NT_SUCCESS(nStatus)){
       printf("**** Error 0x%x returned by BCryptGetProperty when calculating auth tag len\n", nStatus);
   }

   
}


void AESGCM::Decrypt(BYTE* nonce, size_t nonceLen, BYTE* data, size_t dataLen, BYTE* macTag, size_t macTagLen){

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);

    authInfo.pbNonce = nonce;
    authInfo.cbNonce = nonceLen;
    authInfo.pbTag = macTag;
    authInfo.cbTag = macTagLen;
    
    nStatus = BCryptDecrypt(
        hKey, 
        data, dataLen, 
        &authInfo, 
        NULL, 0, 
        NULL, 0, 
        &ptBufferSize, 0);
    if(NT_SUCCESS(nStatus)){ 
        plaintext = (BYTE *) malloc (sizeof (BYTE) * (ptBufferSize+1));
        DWORD dwBytesDone = 0;
        nStatus = BCryptDecrypt(
            hKey, 
            data, dataLen, 
            &authInfo, 
            NULL, 0, 
            plaintext, ptBufferSize, 
            &dwBytesDone, 0);
    }else{
        Cleanup();
    }
}

void AESGCM::Encrypt(BYTE* nonce, size_t nonceLen, BYTE* data, size_t dataLen){
    
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    tag = (BYTE *) malloc (sizeof (BYTE) * (authTagLengths.dwMinLength));

    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);

    authInfo.pbNonce = nonce;
    authInfo.cbNonce = nonceLen;
    authInfo.pbTag = tag;
    authInfo.cbTag = authTagLengths.dwMinLength;

    BYTE* cipherText;
    DWORD cipherLen = 0;
    nStatus = BCryptEncrypt(
        hKey, 
        data, dataLen, 
        &authInfo, 
        NULL, 0, 
        //ciphertext, and buffer len
        NULL, 0, 
        &cipherLen, 0);
    if(NT_SUCCESS(nStatus)){
        ciphertext = (BYTE *) malloc (sizeof (BYTE) * (cipherLen));
        DWORD dwBytesDone = 0;
        nStatus = BCryptEncrypt(hKey, data, dataLen, &authInfo, NULL, 0, ciphertext, cipherLen, &dwBytesDone, 0);
    }else{
        Cleanup();
    }
   
}   



void AESGCM::Cleanup(){
    if(hAlg){
        ::BCryptCloseAlgorithmProvider(hAlg,0);
        hAlg = NULL;
    }
    if(hKey){
        ::BCryptDestroyKey(hKey);
        hKey = NULL;
    }
    if(tag){
        ::HeapFree(GetProcessHeap(), 0, tag);
        tag = NULL;
    }
    if(ciphertext){
        ::HeapFree(GetProcessHeap(), 0, tag);
        ciphertext = NULL;
    }
    if(plaintext){
        ::HeapFree(GetProcessHeap(), 0, plaintext);
        plaintext = NULL;
    }
}