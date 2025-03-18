#include "server.h"
int main() {
    GameServer server(8080);
    server.run();
    return 0;
}
