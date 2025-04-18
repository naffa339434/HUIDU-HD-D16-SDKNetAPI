#include <string>
#include <iostream>
#include <thread>
#include "ICApi.h"
#include <winsock2.h>
#include <ArduinoJson.h>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>

using namespace std;
using namespace ArduinoJson::V731HB42;

// Global event core pointer so we can call Quit from callbacks.
static HEventCore *core = nullptr;

const char* Type = nullptr;
const char* Device = nullptr;
const char* Device_IP = nullptr;
int Device_Port = 0;

std::queue<std::string> messageQueue;
std::mutex queueMutex;
std::condition_variable dataAvailable;
bool stopReceiver = false;

string Gate_TL[1]     = {""};
string Gantry_TL[8]   = {"", "", "", "", "", "", "", ""};
string brightnessData = "100"; // Default brightness value 

string gantry_IN =      R"({
                            "ServertoDevice": {
                                "Type": "Command",
                                "Device": "Gantry In",
                                "IP":"192.168.10.60",
                                "Port":10001,
                                "Data": [
                                { "Name": "Bay 1", "Value": "AVF-461" },
                                { "Name": "Bay 2", "Value": "KRB-462" },
                                { "Name": "Bay 3", "Value": "QCY-863" },
                                { "Name": "Bay 4", "Value": "OAL-464" },
                                { "Name": "Bay 5", "Value": "TL55465" },
                                { "Name": "Bay 6", "Value": "TL65466" },
                                { "Name": "Bay 7", "Value": "TL75467" },
                                { "Name": "Bay 8", "Value": "TL85468" }
                                ]
                            }
                        })";

string gantry_OUT =      R"({
                            "ServertoDevice": {
                                "Type": "Command",
                                "Device": "Gantry Out",
                                "IP":"192.168.10.61",
                                "Port":10001,
                                "Data": [
                                { "Name": "Bay 1", "Value": "TL55461" },
                                { "Name": "Bay 2", "Value": "TL65462" },
                                { "Name": "Bay 3", "Value": "TL75463" },
                                { "Name": "Bay 4", "Value": "TL85464" },
                                { "Name": "Bay 5", "Value": "TL95465" },
                                { "Name": "Bay 6", "Value": "TL05466" },
                                { "Name": "Bay 7", "Value": "TL35467" },
                                { "Name": "Bay 8", "Value": "TL45468" }
                                ]
                            }
                        })";

string gate_IN =        R"({
                            "ServertoDevice": {
                                "Type": "Command",
                                "Device": "Gate In",
                                "IP":"192.168.10.60",
                                "Port":10001,
                                "Data": [
                                { "Name": "Data", "Value": "GXP-468" }
                                ]
                            }
                        })";

string gate_OUT =       R"({
                            "ServertoDevice": {
                                "Type": "Command",
                                "Device": "Gate Out",
                                "IP":"192.168.10.61",
                                "Port":10001,
                                "Data": [
                                { "Name": "Data", "Value": "AXY-683" }
                                ]
                            }
                        })";
                            
string gateConfig  =    R"({
                            "ServertoDevice": {
                                "Type": "Configuration",
                                "Device": "Gate In",
                                "IP":"192.168.10.60",
                                "Port":10001,
                                "Data": [
                                { "Name": "Brightness", "Value": "100" }
                                ]
                            }
                        })";

string gateConfig1 =    R"({
                            "ServertoDevice": {
                                "Type": "Configuration",
                                "Device": "Gate Out",
                                "IP":"192.168.10.61",
                                "Port":10001,
                                "Data": [
                                { "Name": "Brightness", "Value": "100" }
                                ]
                            }
                        })";
                
string gantryResponseCommand = R"({
                                "DevicetoServer": {
                                    "Type": "Response",
                                    "Device": "Gantry In",
                                    "Data": [
                                    { "Name": "Commnad", "Value": "OK/Failed" }
                                    ]
                                }
                                })";

string gantryResponseConfig = R"({
                                "DevicetoServer": {
                                    "Type": "Response",
                                    "Device": "Gantry In",
                                    "Data": [
                                    { "Name": "Configuration", "Value": "OK/Failed" }
                                    ]
                                }
                                })";

#define SERVER_IP           "192.168.10.75" 
#define SERVER_PORT         5005            // The port to listen on (must match server's port)
#define RECONNECT_DELAY_SECONDS 5

#define Gate_IN_Greeting    "پی ایس او فیصل آباد ڈپو میں خوش آمدید"
#define Gate_OUT_Greeting    "پی ایس او فیصل آباد ڈپو خدا حافظ"
//#define Gate_OUT_Greeting   "پی ایس او فیصل آباد ڈپومیں آمد کا شکریہ"
//#define Gate_IN_Greeting    "Welcome to PSO Faisalabad Depot"
//#define Gate_OUT_Greeting   "Thank you for Visiting PSO Faisalabad Depot"

#define XML_Gate            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" \
                            "<sdk guid=\"b04f672504da67f19d48edb3b173c15f\">\n" \
                            "  <in method=\"AddProgram\">\n" \
                            "    <screen timeStamps=\"1744104321754\">\n" \
                            "      <program type=\"normal\" id=\"0\" guid=\"b41bbdb7-1b15-4360-9127-2f25dc3ec2b9\" name=\"\">\n" \
                            "        <playControl count=\"1\" disabled=\"false\" />\n" \
                            "        <area guid=\"78876cc1-b5f7-4b9c-8340-a50fa895a5ef\" name=\"\" alpha=\"255\">\n" \
                            "          <rectangle x=\"0\" y=\"5\" width=\"256\" height=\"64\" />\n" \
                            "          <resources>\n" \
                            "            <text guid=\"766942a6-5b3f-46e0-ac44-6560c3259063\" name=\"\" singleLine=\"false\">\n" \
                            "              <style align=\"center\"/>\n" \
                            "              <string>%s</string>\n" \
                            "              <font name=\"Arial\" size=\"24\" color=\"#00b8c7\" bold=\"true\" italic=\"false\" underline=\"false\" />\n" \
                            "              <effect in=\"0\" inSpeed=\"4\" out=\"20\" outSpeed=\"4\" duration=\"0\" />\n" \
                            "            </text>\n" \
                            "          </resources>\n" \
                            "        </area>\n" \
                            "        <area alpha=\"255\" guid=\"areaclockGuid1\">\n" \
                            "          <rectangle x=\"0\" y=\"65\" width=\"256\" height=\"60\"/>\n" \
                            "          <resources>\n" \
                            "            <text singleLine=\"true\">\n" \
                            "              <style align=\"center\" />\n" \
                            "              <font name=\"Arial\" size=\"55\" color=\"#00b8c7\" bold=\"true\" italic=\"false\" underline=\"false\" />\n" \
                            "              <effect duration=\"100\" in=\"0\" inSpeed=\"4\" outSpeed=\"4\" out=\"20\"/>\n" \
                            "              <string>%s</string>\n" \
                            "            </text>\n" \
                            "          </resources>\n" \
                            "        </area>\n" \
                            "      </program>\n" \
                            "    </screen>\n" \
                            "  </in>\n" \
                            "</sdk>"   

#define XML_Gantry          "<?xml version='1.0' encoding='utf-8'?>\n" \
                            "<sdk guid=\"##GUID\">\n" \
                            "    <in method=\"AddProgram\">\n" \
                            "        <screen timeStamps=\"12311454\">\n" \
                            "            <program guid=\"{41d60f81-f3d5-4357-94fe-8bbaf158d06c}\">\n" \
                            "                <playControl count=\"1\"/>\n" \
                            "                <area alpha=\"255\" guid=\"areaclockGuid1\">\n" \
                            "                    <rectangle x=\"0\" y=\"0\" width=\"256\" height=\"35\" />\n" \
                            "                    <resources>\n" \
                            "                        <text singleLine=\"true\">\n" \
                            "                            <font bold=\"true\" size=\"30\" color=\"#00b8c7\" />\n" \
                            "                            <effect duration=\"100\" in=\"0\" inSpeed=\"4\" outSpeed=\"4\" out=\"20\"/>\n" \
                            "                            <style align=\"left\"/>\n" \
                            "                            <string> Bay1 %s</string>\n" \
                            "                        </text>\n" \
                            "                    </resources>\n" \
                            "                </area>\n" \
                            "                <area alpha=\"255\" guid=\"areaclockGuid2\">\n" \
                            "                    <rectangle x=\"0\" y=\"30\" width=\"256\" height=\"35\" />\n" \
                            "                    <resources>\n" \
                            "                        <text singleLine=\"true\">\n" \
                            "                            <font bold=\"true\" size=\"30\" color=\"#00b8c7\" />\n" \
                            "                            <effect duration=\"100\" in=\"0\" inSpeed=\"4\" outSpeed=\"4\" out=\"20\"/>\n" \
                            "                            <style align=\"left\"/>\n" \
                            "                            <string> Bay2 %s</string>\n" \
                            "                        </text>\n" \
                            "                    </resources>\n" \
                            "                </area>\n" \
                            "                <area alpha=\"255\" guid=\"areaclockGuid3\">\n" \
                            "                    <rectangle x=\"0\" y=\"60\" width=\"256\" height=\"35\" />\n" \
                            "                    <resources>\n" \
                            "                        <text singleLine=\"true\">\n" \
                            "                            <font bold=\"true\" size=\"30\" color=\"#00b8c7\" />\n" \
                            "                            <effect duration=\"100\" in=\"0\" inSpeed=\"4\" outSpeed=\"4\" out=\"20\"/>\n" \
                            "                            <style align=\"left\"/>\n" \
                            "                            <string> Bay3 %s</string>\n" \
                            "                        </text>\n" \
                            "                    </resources>\n" \
                            "                </area>\n" \
                            "                <area alpha=\"255\" guid=\"areaclockGuid4\">\n" \
                            "                    <rectangle x=\"0\" y=\"90\" width=\"256\" height=\"35\" />\n" \
                            "                    <resources>\n" \
                            "                        <text singleLine=\"true\">\n" \
                            "                            <font bold=\"true\" size=\"30\" color=\"#00b8c7\" />\n" \
                            "                            <effect duration=\"100\" in=\"0\" inSpeed=\"4\" outSpeed=\"4\" out=\"20\"/>\n" \
                            "                            <style align=\"left\"/>\n" \
                            "                            <string> Bay4 %s</string>\n" \
                            "                        </text>\n" \
                            "                    </resources>\n" \
                            "                </area>\n" \
                            "            </program>\n" \
                            "        </screen>\n" \
                            "    </in>\n" \
                            "</sdk>"

#define brightnessXML       "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" \
                            "<sdk guid=\"\">\n" \
                            "  <in method=\"SetLuminancePloy\">\n" \
                            "    <mode value=\"default\" />\n" \
                            "    <default value=\"%s\" />\n" \
                            "    <ploy />\n" \
                            "    <sensor min=\"1\" max=\"100\" time=\"5\" />\n" \
                            "  </in>\n" \
                            "</sdk>"
                        
#define GETDEVICEINFO_TEXT  "<?xml version=\'1.0\' encoding=\'utf-8\'?>\n" \
                            "<sdk guid=\"##GUID\">\n"                      \
                            "    <in method=\"GetDeviceInfo\"/>\n"         \
                            "</sdk>"

static void ReadData(HSession *currSession, const char *data, huint32 len, void *userData) {
    printf("--------------Read Data:--------------\n");   
    printf("  currSession: %p,\t userData: %p,\t len: %u\n", (void*)currSession, userData, (unsigned int)len);
    printf("  data:        %s\n", data);
    printf("-------------------------------------\n");
    Quit(core);
}

static void DebugLog(HSession *currSession, const char *log, huint32 len, void *userData) {
    printf("--------------Debug log:--------------\n");
    printf("  currSession: %p,\t userData: %p,\t len: %u\n", (void*)currSession, userData, (unsigned int)len);
    printf("  log:         %s\n", log);   
    printf("-------------------------------------\n");
    fflush(stdout);
}

static void NetStatus(HSession *currSession, eNetStatus status, void *userData) {
    printf("--------------NetStatus:--------------\n");
    printf("  currSession: %p,\t userData: %p\n", (void*)currSession, userData);
    printf("  NetStatus:   %d\n", status); 
    printf("-------------------------------------\n");

    switch (status) {
    case kConnect:
        printf("NetStatus : Device Connected\n");
        break;
    case kDisconnect:
        printf("NetStatus : Device Disconnect\n");
        break;
    default:
        break;
    }
    fflush(stdout);
}
// Callback function to handle detected device information
// static void DeviceInfo(HSession *currSession, const char *id, huint32 idLen, const char *ip, huint32 ipLen, const char *readData, huint32 dataLen, void *userData) {
//     printf("id[%s], ip[%s]\n", id, ip);  // Print device ID and IP address
// }

static void DeviceInfo(HSession *currSession, const char *id, huint32 idLen, const char *ip, huint32 ipLen, const char *readData, huint32 dataLen, void *userData) {
    printf("--------------DeviceInfo:--------------\n");
    printf("  currSession: %p\n", (void*)currSession);
    printf("  id:          %.*s\n", (int)idLen, id);
    printf("  idLen:       %u\n", (unsigned int)idLen);
    printf("  ip:          %.*s\n", (int)ipLen, ip);
    printf("  ipLen:       %u\n", (unsigned int)ipLen);
    printf("  readData:    %.*s\n", (int)dataLen, readData);
    printf("  dataLen:     %u\n", (unsigned int)dataLen);
    printf("  userData:    %p\n", userData);
    printf("-------------------------------------\n");
}

void Display(HSession* session) {
    char finalXml[4096];
    const char* xmlTemplate = nullptr;
    
    if (strcmp(Device, "Gate In") == 0)
    {
        xmlTemplate = XML_Gate; //Macro to define the XML template
        snprintf(finalXml, sizeof(finalXml), xmlTemplate, Gate_IN_Greeting, Gate_TL[0].c_str()); // Construct the XML command
    }
    else if (strcmp(Device, "Gate Out") == 0)
    {
        xmlTemplate = XML_Gate; //Macro to define the XML template
        snprintf(finalXml, sizeof(finalXml), xmlTemplate, Gate_OUT_Greeting, Gate_TL[0].c_str()); // Construct the XML command  
    }
    else if (strcmp(Device, "Gantry In") == 0)
    {
        xmlTemplate = XML_Gantry;
        snprintf(finalXml, sizeof(finalXml), xmlTemplate, Gantry_TL[0].c_str(),Gantry_TL[1].c_str(),Gantry_TL[2].c_str(),Gantry_TL[3].c_str()); // Construct the XML command  
    }
    else if (strcmp(Device, "Gantry Out") == 0)
    {
        xmlTemplate = XML_Gantry;
        snprintf(finalXml, sizeof(finalXml), xmlTemplate, Gantry_TL[0].c_str(),Gantry_TL[1].c_str(),Gantry_TL[2].c_str(),Gantry_TL[3].c_str()); // Construct the XML command  
    }
    else {
        printf("Unknown Device Type\n");
        return;
    }
    
    if (Connect(session, Device_IP, Device_Port)) {        
        if (SendSDK(session, finalXml, strlen(finalXml))) {
            
            cout<<finalXml<<endl;
            printf("Text Updated Successfully.\n");
        } else {
            printf("Failed to update display.\n");
        }
    } else {
        printf("Connection to the device failed.\n");
    }
    Exec(core);  // Blocks the main thread and waits for Quit to be called from ReadData callback
}

void HandleBrightness(HSession* session) {
    char finalXml[310] = ""; // Ensure it's large enough

    const char* xmlTemplate = brightnessXML; // Macro to define the XML template
    snprintf(finalXml, sizeof(finalXml), xmlTemplate, brightnessData.c_str()); // Construct the XML command
    // Construct the brightness XML command.
    if (Connect(session, Device_IP, Device_Port)) {
        // Send the XML command over the session
        if (SendSDK(session, finalXml, strlen(finalXml))) {
            printf("Brightness Updated Successfully.\n");
        } else {
            printf("Failed to update brightness.\n");
        }
    } else {
        printf("Connection to the device failed.\n");
    }
    Exec(core); 
}

int deserializeJson(string json) {
    int i = 0;
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        cout << "deserializeJson() failed: " << error.c_str() << endl;
        return 0;
    }

    JsonObject ServertoDevice = doc["ServertoDevice"];
    
    Type = ServertoDevice["Type"];              // "Command/Configuration"
    Device = ServertoDevice["Device"];          // "Gate In/Gate Out/Gantry In/Gantry Out"
    Device_IP = ServertoDevice["IP"];           // "192.168.10.65"
    Device_Port = ServertoDevice["Port"];       // 10001

    JsonArray dataArray = ServertoDevice["Data"].as<JsonArray>();  // Store the temporary in a variable
    for (JsonObject data : dataArray) // Iterate over the array of objects
    {
        const char* Name = data["Name"];        // "Bay 1", ...
        const char* Value = data["Value"];

        // Validate: check if Value exists or is null
        if (Value == nullptr || data["Value"].isNull()) {
            printf("Warning: 'Value' for item at index: %d is null; replacing with 0 \n",i);
            Value = "0"; // Set Value to an empty string if null
        }

        if ((strcmp(Type, "Command") == 0) && ((strcmp(Device, "Gantry In") == 0) || (strcmp(Device, "Gantry Out") == 0))) 
        {
            Gantry_TL[i] = Value;
        } 
    
        else if ((strcmp(Type, "Command") == 0) && ((strcmp(Device, "Gate In") == 0) || (strcmp(Device, "Gate Out") == 0))) 
        {
            Gate_TL[i] = Value;
        } 
        else if ((strcmp(Type, "Configuration") == 0) && (strcmp(Name, "Brightness") == 0)) 
        {
            brightnessData = Value; // Update the brightnessData variable with the new value
        } 
        i++;
    }       
    return 1;
}

// -------------------------------------------------------------------------------------------------------------------------------
SOCKET CreateAndBindSocket() {
    SOCKET sock;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        return INVALID_SOCKET;
    }

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed!" << std::endl;
        closesocket(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

void SetSocketNonBlocking(SOCKET sock) {
    u_long mode = 1; // 1 to enable non-blocking mode
    if (ioctlsocket(sock, FIONBIO, &mode) != NO_ERROR) {
        std::cerr << "Failed to set socket to non-blocking mode!" << std::endl;
        closesocket(sock);
        WSACleanup();
        exit(-1);
    }
}

void InitializeWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        exit(-1);
    }
}

void Cleanup(SOCKET sock) {
    closesocket(sock);
    WSACleanup();
}

std::vector<char> getBuffer(SOCKET sock) {
    struct sockaddr_in client_addr;
    std::vector<char> buffer(2048); // same as char buffer[2048];

    int addr_len = sizeof(client_addr);
    std::string lastMessage;

    while (true) {
        int n = recvfrom(sock, buffer.data(), buffer.size(), 0, (struct sockaddr *)&client_addr, &addr_len);
        if (n == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
                std::cerr << "Socket error: " << err << std::endl;
                break;
            }
        } else {
            buffer[n] = '\0';  // Null-terminate the received string
            std::string currentMessage(buffer.data(), n);
            return buffer; // Return the received message

            
            }

        }

        // Sleep briefly to prevent 100% CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }


void ReconnectSocket(SOCKET &sock) {
    // Close the existing socket
    closesocket(sock);

    // Wait before attempting to reconnect
    std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_DELAY_SECONDS));

    // Recreate and rebind the socket
    sock = CreateAndBindSocket();
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        exit(-1);
    }

    // Set the socket to non-blocking mode
    SetSocketNonBlocking(sock);
}

// --------------------------------------------------------------------------------------------------
void udpReceiver(SOCKET sock) {
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    std::vector<char> buffer(2048);

    while (!stopReceiver) {
        int n = recvfrom(sock, buffer.data(), buffer.size(), 0,
                         (struct sockaddr *)&client_addr, &addr_len);

        if (n > 0) {
            std::string msg(buffer.data(), n);

            std::unique_lock<std::mutex> lock(queueMutex);
            messageQueue.push(msg);
            dataAvailable.notify_one();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void processMessages() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        dataAvailable.wait(lock, [] { return !messageQueue.empty() || stopReceiver; });

        while (!messageQueue.empty()) {
            std::string msg = messageQueue.front();
            messageQueue.pop();
            lock.unlock();

            std::cout << "Main thread got message: " << msg << std::endl;

            lock.lock();
        }

        if (stopReceiver) break;
    }
}



int main() {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in server_addr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return -1;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        WSACleanup();
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed!" << std::endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    std::thread receiver(udpReceiver, sock);
    std::thread processor(processMessages);

    std::this_thread::sleep_for(std::chrono::seconds(60)); // Example run time
    stopReceiver = true;
    dataAvailable.notify_all();

    receiver.join();
    processor.join();

    closesocket(sock);
    WSACleanup();
    return 0;
}
