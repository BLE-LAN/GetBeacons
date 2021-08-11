#pragma once

#include <../winrt/windows.devices.bluetooth.advertisement.h>

using namespace ABI::Windows::Devices::Bluetooth::Advertisement;

namespace Watcher {
    struct CallBackObject
    {
        CallBackObject() {};
        HRESULT AdvertisementRecived(IBluetoothLEAdvertisementWatcher* watcher, IBluetoothLEAdvertisementReceivedEventArgs* args);
    };

    void Run(unsigned int miliseconds, std::string outputFile);
    
    int WatchADV(unsigned int miliseconds);
}


