#include <cstdint>
#include <string>
#include <netinet/in.h>

using namespace std;

enum class CONN_STATE : uint32_t {
    CLOSED,
    OPEN
};

class Connection {
public:
    Connection(string host);
    ~Connection();

    void sendData(void* buff, uint32_t buffsize);
    uint32_t recvData(void* buff, uint32_t buffsize);

    void closeConnection();

private:
    CONN_STATE state_;
    uint32_t socket_;

    // dns hostname 
    string destination_;
};
