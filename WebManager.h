#ifndef WEB_MANAGER_H
#define WEB_MANAGER_H

#include <FS.h>
// Fix for ESP32 Core 3.x where FS is in fs namespace but WebServer expects global
using fs::FS;
#include <WebServer.h>
#include "NetworkManager.h"

class WebManager {
private:
    WebServer server;
    SENetworkManager* netMgr;

public:
    WebManager(SENetworkManager* nm);
    void begin();
    void update();
    void handleRoot();
};

#endif
