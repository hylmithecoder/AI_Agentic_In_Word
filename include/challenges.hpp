#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <json.hpp>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <debugger.hpp>
#include <random>
#include <uuid/uuid.h>
using namespace Debug;
using namespace std;
using namespace nlohmann;

// Helper: base64 encode
namespace HelperChallenges {
    string base64Encode(const string& input);

    class Challenges {
    public:
        static string encode(const nlohmann::json& e) {
            string dumped = e.dump(); // compact JSON
            return base64Encode(dumped);
        }

        static string mod(const string& e) {
            uint32_t t = 2166136261u;
            for (char ch : e) {
                t ^= static_cast<uint32_t>(static_cast<unsigned char>(ch));
                t = (t * 16777619u) & 0xFFFFFFFFu;
            }
            t ^= (t >> 16);
            t = (t * 2246822507u) & 0xFFFFFFFFu;
            t ^= (t >> 13);
            t = (t * 3266489909u) & 0xFFFFFFFFu;
            t ^= (t >> 16);

            ostringstream oss;
            oss << hex << setw(8) << setfill('0') << t;
            return oss.str();
        }

        static string runCheck(long long t0,
                            const string& n,
                            const string& r,
                            int o,
                            nlohmann::json& config) {
            config[3] = o;
            auto now = chrono::system_clock::now();
            auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count();
            config[9] = static_cast<int>(ms - t0);

            string i = encode(config);

            if (mod(n + i).substr(0, r.size()) <= r) {
                return i + "~S";
            }
            return "";
        }

        string uuid4() {
            uuid_t id;
            char sid_str[37]; // UUID string = 36 chars + null terminator

            // generate UUID v4
            uuid_generate_random(id);

            // convert ke string
            uuid_unparse(id, sid_str);

            return string(sid_str);
        }

        float randomFloat();
        // helper random int range
        int randomInt(int min, int max);
        // timestamp string ala Python strftime
        string getCurrentTimestamp();
        string getCurrentTimezoneString();
        json CreateConfig(const string& prod, const string& userAgent);
    };
}