#include "Server/Server.hpp"

bool checkPort(const std::string& str) {
    if (str.empty()) 
        return false;
    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        if (!std::isdigit(*it))
            return false;
    }
    return true;
}


int main(int argc , char **argv) {

    if (argc != 3)
    {
        std::cerr<< "Usage" << argv[0]<< " <port> <password>"<< std::endl;
        return 1;
    }
    if(checkPort(argv[1]) == false)
    {
        std::cerr<< "Invalid port"<< std::endl;
        return 1;
    }
    int port = std::atoi(argv[1]);
    std::string password = argv[2];
    if (port < 1024 || port > 65535) {
        std::cerr << "Error: Port must be a number between 1024 and 65535." << std::endl;
        return 1;
    }
    try {
        Server srv(port, password);

        std::cout << "SUCCESS: Server object was created and socket was set up." << std::endl;
        srv.run();
    } catch (const std::exception& e) {
        std::cerr << "FAILURE: The server could not be created." << std::endl;
        std::cerr << "Reason: " << e.what() << std::endl;
    }
     
    std::cout << "--- Test Finished ---" << std::endl;
    return 0;
}