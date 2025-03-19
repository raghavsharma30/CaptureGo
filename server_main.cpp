#include "server.h"
using namespace std;
int main() {
    GameServer server(8080);
    server.run();
    return 0;
}
