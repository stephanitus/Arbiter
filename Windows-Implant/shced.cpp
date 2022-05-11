/********************************************************************
 This sample schedules a task to start Notepad.exe 30 seconds after
 the system is started. 
********************************************************************/
#ifndef PERSIST_H
#define PERSIST_H



#define _WIN32_DCOM

#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <comdef.h>
//  Include the task header file.
#include <taskschd.h>
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")
//todo: 
// [x] change all error statements
// make header file
// [x] make input string
#endif
using namespace std;

string __cdecl persistMe(wstring path)
{
    //  ------------------------------------------------------
    //  Initialize COM.
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if( FAILED(hr) )
    {
       
        return  "\nCoInitializeEx failed: %x", hr;
    }

    //  Set general COM security levels.
    hr = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        0,
        NULL);

    if( FAILED(hr) )
    {
        CoUninitialize();
        return "CoInitializeSecurity failed: "+ to_string(hr);
    }

    //  ------------------------------------------------------
    //  Create a name for the task.
    LPCWSTR wszTaskName = L"Boot Trigger Test Task";

    //  Get the Windows directory and set the path to Notepad.exe.
    wstring wstrExecutablePath = path


    //  ------------------------------------------------------
    //  Create an instance of the Task Service. 
    ITaskService *pService = NULL;
    hr = CoCreateInstance( CLSID_TaskScheduler,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_ITaskService,
                           (void**)&pService );  
    if (FAILED(hr))
    {
         
          CoUninitialize();
          return "Failed to create an instance of ITaskService: " +to_string(hr);
    }
        
    //  Connect to the task service.
    hr = pService->Connect(_variant_t(), _variant_t(),
        _variant_t(), _variant_t());
    if( FAILED(hr) )
    {
        
        pService->Release();
        CoUninitialize();
        return "ITaskService::Connect failed: " + to_string(hr);
    }

    //  ------------------------------------------------------
    //  Get the pointer to the root task folder.  
    //  This folder will hold the new task that is registered.
    ITaskFolder *pRootFolder = NULL;
    hr = pService->GetFolder( _bstr_t( L"\\") , &pRootFolder );
    if( FAILED(hr) )
    {
        
        pService->Release();
        CoUninitialize();
        return "Cannot get Root Folder pointer: " + to_string(hr);
    }
    
    //  If the same task exists, remove it.
    pRootFolder->DeleteTask( _bstr_t( wszTaskName), 0  );
    
    //  Create the task builder object to create the task.
    ITaskDefinition *pTask = NULL;
    hr = pService->NewTask( 0, &pTask );

    pService->Release();  // COM clean up.  Pointer is no longer used.
    if (FAILED(hr))
    {
          
          pRootFolder->Release();
          CoUninitialize();
          return "Failed to create a task definition: "+ to_string(hr);
    }
    
        
    //  ------------------------------------------------------
    //  Get the registration info for setting the identification.
    IRegistrationInfo *pRegInfo= NULL;
    hr = pTask->get_RegistrationInfo( &pRegInfo );
    if( FAILED(hr) )
    {
        
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return "Cannot get identification pointer: " + to_string(hr);
    }
    
    hr = pRegInfo->put_Author(L"Author Name");
    pRegInfo->Release();
    if( FAILED(hr) )
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return "Cannot put identification info: " + to_string(hr);
    }

    //  ------------------------------------------------------
    //  Create the settings for the task
    ITaskSettings *pSettings = NULL;
    hr = pTask->get_Settings( &pSettings );
    if( FAILED(hr) )
    {
        
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return "Cannot get settings pointer: " + to_string(hr);
    }
    
    //  Set setting values for the task. 
    hr = pSettings->put_StartWhenAvailable(VARIANT_TRUE);
    pSettings->Release();
    if( FAILED(hr) )
    {
        
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return "Cannot put setting info: " + to_string(hr);
    }
       

    //  ------------------------------------------------------
    //  Get the trigger collection to insert the boot trigger.
    ITriggerCollection *pTriggerCollection = NULL;
    hr = pTask->get_Triggers( &pTriggerCollection );
    if( FAILED(hr) )
    {
        
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return "Cannot get trigger collection: "+ to_string(hr);
    }

    //  Add the boot trigger to the task.
    ITrigger *pTrigger = NULL;
    hr = pTriggerCollection->Create( TASK_TRIGGER_BOOT, &pTrigger ); 
    pTriggerCollection->Release();
    if( FAILED(hr) )
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return "Cannot create the trigger: "+ to_string(hr);
    }
    
    IBootTrigger *pBootTrigger = NULL;
    hr = pTrigger->QueryInterface( 
        IID_IBootTrigger, (void**) &pBootTrigger );
    pTrigger->Release();
    if( FAILED(hr) )
    {
        
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return "QueryInterface call failed for IBootTrigger: " + to_string(hr);
    }

    hr = pBootTrigger->put_Id( _bstr_t( L"Trigger1" ) );
    //  Set the task to start at a certain time. The time 
    //  format should be YYYY-MM-DDTHH:MM:SS(+-)(timezone).
    //  For example, the start boundary below
    //  is January 1st 2005 at 12:05
    hr = pBootTrigger->put_StartBoundary( _bstr_t(L"2022-04-30T12:00:00") );
    if( FAILED(hr) )
       return "Cannot put the start boundary: " + to_string(hr);
  
    hr = pBootTrigger->put_EndBoundary( _bstr_t(L"2022-07-30T12:00:00") );
    if( FAILED(hr) )
       return "Cannot put the end boundary: " + to_string(hr);

    // Delay the task to start 30 seconds after system start. 
    hr = pBootTrigger->put_Delay( L"PT30S" );
    pBootTrigger->Release();
    if( FAILED(hr) )
    {
        
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return "Cannot put delay for boot trigger: " + std::to_string(hr) ;
    } 
       

    //  ------------------------------------------------------
    //  Add an Action to the task. This task will execute Notepad.exe.     
    IActionCollection *pActionCollection = NULL;

    //  Get the task action collection pointer.
    hr = pTask->get_Actions( &pActionCollection );
    if( FAILED(hr) )
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return "nCannot get Task collection pointer: "  + std::to_string(hr);
    }
        
    //  Create the action, specifying it as an executable action.
    IAction *pAction = NULL;
    hr = pActionCollection->Create( TASK_ACTION_EXEC, &pAction );
    pActionCollection->Release();
    if( FAILED(hr) )
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return "Cannot create the action: " + std::to_string(hr);
    }

    IExecAction *pExecAction = NULL;
    //  QI for the executable task pointer.
    hr = pAction->QueryInterface( 
        IID_IExecAction, (void**) &pExecAction );
    pAction->Release();
    if( FAILED(hr) )
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return "QueryInterface call failed for IExecAction: "  + std::to_string(hr);
    }

    //  Set the path of the executable to Notepad.exe.
    hr = pExecAction->put_Path( _bstr_t( wstrExecutablePath.c_str() ) ); 
    pExecAction->Release(); 
    if( FAILED(hr) )
    {
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return "Cannot set path of executable: "+ std::to_string(hr);
    }
      
    
    //  ------------------------------------------------------
    //  Save the task in the root folder.
    IRegisteredTask *pRegisteredTask = NULL;
    VARIANT varPassword;
    varPassword.vt = VT_EMPTY;
    hr = pRootFolder->RegisterTaskDefinition(
            _bstr_t( wszTaskName ),
            pTask,
            TASK_CREATE_OR_UPDATE, 
            _variant_t(L"Local Service"), 
            varPassword, 
            TASK_LOGON_SERVICE_ACCOUNT,
            _variant_t(L""),
            &pRegisteredTask);
    if( FAILED(hr) )
    {
        
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return "Error saving the Task : " + std::to_string(hr);
    }
    

    //  Clean up.
    pRootFolder->Release();
    pTask->Release();
    pRegisteredTask->Release();
    CoUninitialize();
    return "Success! Task successfully registered. "
;
}