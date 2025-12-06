#ifndef WEB_MANAGER_H
#define WEB_MANAGER_H

#include <FS.h>
// FS is in fs namespace but WebServer expects global lmao why did this take this long to figure out
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
