#include <iostream>
#include <rpcdce.h>
#include <string>
using namespace std;

#pragma comment(lib, "Rpcrt4.lib")

namespace SecurityHandler {
class Security {
public:
  string UUIDGenerator();
};
} // namespace SecurityHandler