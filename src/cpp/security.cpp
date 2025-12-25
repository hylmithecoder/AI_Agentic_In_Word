#include "security.hpp"
// #include <rpcdce.h>

#pragma comment(lib, "Rpcrt4.lib")

namespace SecurityHandler {

string Security::UUIDGenerator() {
  UUID uuid;
  UuidCreate(&uuid); // Generates a new UUID

  // Convert the binary UUID to a string format
  RPC_CWSTR uuidString = nullptr;
  UuidToString(&uuid, &uuidString); // Allocates memory for the string

  if (uuidString != nullptr) {
    std::cout << "Generated UUID: " << (char *)uuidString << std::endl;

    // The application is responsible for freeing the allocated memory
    RpcStringFree(&uuidString);
  } else {
    std::cerr << "Failed to generate UUID string." << std::endl;
  }
}
}; // namespace SecurityHandler