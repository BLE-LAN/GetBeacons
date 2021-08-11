

#include <iostream>
#include <functional>
#include <windows.h>
#include <stdio.h>
#include <thread>
// required Windows Runtime
#include <Windows.Foundation.h>
#include <wrl/wrappers/corewrappers.h>
#include <wrl/client.h>
#include <wrl/event.h>
// Sin los .. usa cppwinrt
#include <../winrt/windows.devices.bluetooth.h>

#include "Logger.hpp"
#include "Parser.h"
#include "Watcher.h"

using namespace std;
using namespace ABI::Windows::Foundation;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

#define error(message) logError("WATCHER", message)
#define info(message) logInfo("WATCHER", message)

#define MAX_JSON_BUFFER_SIZE 512

ComPtr<IBluetoothLEAdvertisementWatcher> _WATCHER;
FILE* _FILEPTR;

 HRESULT Watcher::CallBackObject::AdvertisementRecived(IBluetoothLEAdvertisementWatcher* watcher, IBluetoothLEAdvertisementReceivedEventArgs* args)
 {
     char buffer[MAX_JSON_BUFFER_SIZE];
     memset(buffer, 0, MAX_JSON_BUFFER_SIZE);
     if (!Parser::Parser(args, buffer, MAX_JSON_BUFFER_SIZE))
     {
         error("EventArgs no se ha podido parsear");
     }
     else {
         fwrite(buffer, sizeof(char), strlen(buffer), _FILEPTR);
         fprintf(_FILEPTR, "\n");
     }
     return S_OK;
 }

 void Watcher::Run(unsigned int miliseconds, std::string outputFile)
 {
     std::clog << "###################################\n";
     info("Run()");

     fopen_s(&_FILEPTR, outputFile.c_str(), "w");

     if (_FILEPTR == NULL)
     {
         error("Cloud not open file '" + outputFile + "'");
     }
     else 
     {
         Watcher::WatchADV(miliseconds);
         fclose(_FILEPTR);
     }
 }

int Watcher::WatchADV(unsigned int timeToWatch)
{
    EventRegistrationToken* watcherToken = new EventRegistrationToken();
    HRESULT hr;

    RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
    if (FAILED(initialize)) {
        error("RoInitializeWrapper failed");
        return -1;
    }

    ComPtr<IBluetoothLEAdvertisementWatcherFactory> bleAdvWatcherFactory;
    hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_Devices_Bluetooth_Advertisement_BluetoothLEAdvertisementWatcher).Get(), &bleAdvWatcherFactory);
    if (FAILED(hr)) {
        error("GetActivationFactory failed");
        return -1;
    }

    //ComPtr<IBluetoothLEAdvertisementWatcher> bleWatcher;
    ComPtr<IBluetoothLEAdvertisementFilter> bleFilter;

    Wrappers::HStringReference class_id_filter(RuntimeClass_Windows_Devices_Bluetooth_Advertisement_BluetoothLEAdvertisementFilter);
    
    hr = RoActivateInstance(class_id_filter.Get(), reinterpret_cast<IInspectable**>(bleFilter.GetAddressOf()));
    if (FAILED(hr)) {
        error("RoActivateInstance error");
        return -1;
    }
    
    hr = bleAdvWatcherFactory->Create(bleFilter.Get(), _WATCHER.GetAddressOf());
    if (_WATCHER == NULL || (FAILED(hr))) {
        error("Create Watcher error");
        return -1;
    }
   
    struct CallBackObject object;
    ComPtr<ITypedEventHandler<BluetoothLEAdvertisementWatcher*, BluetoothLEAdvertisementReceivedEventArgs*>> handler;

    handler = Callback<ITypedEventHandler<BluetoothLEAdvertisementWatcher*, BluetoothLEAdvertisementReceivedEventArgs*> >
        (std::bind(
            &CallBackObject::AdvertisementRecived,
            &object,
            placeholders::_1,
            placeholders::_2
        ));
        
    hr = _WATCHER->add_Received(handler.Get(), watcherToken);
    if (FAILED(hr)) {
        error("Add received callback error");
        return -1;
    }

    _WATCHER->Start();
    info("watcher started");
    
    // sleep main thread and let the callback running
    Sleep(timeToWatch);

    _WATCHER->Stop();
    info("watcher stopped");

    return true;
}