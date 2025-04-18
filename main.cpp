#include <string>
#include <iostream>
#include <thread>
#include "ICApi.h"
#include <winsock2.h>
#include <ArduinoJson.h>
#include "tinyxml2.h"
#include <windows.h>
#include <fcntl.h>

using namespace std;
using namespace ArduinoJson::V731HB42;

// Global event core pointer so we can call Quit from callbacks.
static HEventCore *core = nullptr;

const char* Type = nullptr;
const char* Device = nullptr;
const char* Device_IP = nullptr;
int Device_Port = 0;
volatile bool g_shutdown = false;

string Gate_TL[1]     = {""};
string Gantry_TL[8]   = {"", "", "", "", "", "", "", ""};
string brightnessData = "100";      // Default brightness value 

const char* Name = nullptr;         // This is the variable that will be receiving the settings of what to do with the data received exmaple change the brioghtness or turn the display on or off
const char* ScreenFunc= nullptr;    // This is the variable that will be receiving the settings of what to do with the data received exmaple change the brioghtness or turn the display on or off
const char* IP2Set = nullptr;
string IPData = "";

#define IP "192.168.10.60"

string gantry_IN =      R"({
                            "ServertoDevice": {
                                "Type": "Command",
                                "Device": "Gantry In",
                                "IP":"192.168.10.27",
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
                                "IP":"192.168.10.27",
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
                                "IP":"192.168.10.27",
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
                                "IP":"192.168.10.27",
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
                                "IP":"192.168.10.27",
                                "Port":10001,
                                "Data": [
                                { "Name": "Brightness", "Value": "5" }
                                ]
                            }
                        })";

string gateConfig1 =    R"({
                            "ServertoDevice": {
                                "Type": "Configuration",
                                "Device": "Gate Out",
                                "IP":"192.168.10.27",
                                "Port":10001,
                                "Data": [
                                { "Name": "Brightness", "Value": "10" }
                                ]
                            }
                        })";
                        
string ConfigReboot =    R"({
                            "ServertoDevice": {
                                "Type": "Configuration",
                                "Device": "Gate Out",
                                "IP":"192.168.10.60",
                                "Port":10001,
                                "Data": [
                                { "Name": "Reboot", "Value": "Reboot" }
                                ]
                            }
                        })";

string ConfigPowerOn =    R"({
                            "ServertoDevice": {
                                "Type": "Configuration",
                                "Device": "Gate Out",
                                "IP":"192.168.10.27",
                                "Port":10001,
                                "Data": [
                                { "Name": "Screen", "Value": "OpenScreen" }
                                ]
                            }
                        })";

string ConfigPowerOff = R"({
                            "ServertoDevice": {
                                "Type": "Configuration",
                                "Device": "Gate Out",
                                "IP":"192.168.10.27",
                                "Port":10001,
                                "Data": [
                                { "Name": "Screen", "Value": "CloseScreen" }
                                ]
                            }
                        })";


string SetIPCommand = R"({
                    "ServertoDevice": {
                        "Type": "Configuration",
                        "Device": "Gate Out",
                        "IP":"192.168.10.62",
                        "Port":10001,
                        "Data": [
                        { "Name": "SetIP", "Value": "192.168.10.60" }
                        ]
                    }
                })";

string GetIPCommand = R"({
                    "ServertoDevice": {
                        "Type": "Configuration",
                        "Device": "Gate Out",
                        "IP":"192.168.10.27",
                        "Port":10001,
                        "Data": [
                        { "Name": "GetIP", "Value": "" }
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

#define XML_SCREEN          "<?xml version='1.0' encoding='utf-8'?>\n" \
                            "<sdk guid=\"##GUID\">\n"                   \
                            "    <in method=\"%s\"/>\n"        \
                            "</sdk>"
  
#define GET_ETH0_INFO_XML  "<?xml version='1.0' encoding='utf-8'?>\n" \
                            "<sdk guid=\"##GUID\">\n" \
                            "    <in method=\"GetEth0Info\"/>\n" \
                            "</sdk>"

#define SET_ETH0_XML    "<?xml version='1.0' encoding='utf-8'?>\n" \
                            "<sdk guid=\"##GUID\">\n" \
                            "    <in method=\"SetEth0Info\">\n" \
                            "        <eth valid=\"true\">\n" \
                            "            <enable value=\"true\"/>\n" \
                            "            <dhcp auto=\"false\"/>\n" \
                            "            <address ip=\"%s\" netmask=\"255.255.255.0\" gateway=\"192.168.10.1\" dns=\"192.168.10.1\"/>\n" \
                            "        </eth>\n" \
                            "    </in>\n" \
                            "</sdk>"

#define GET_DEVICE_INFO     "<?xml version='1.0' encoding='utf-8'?>\n" \
                            "<sdk guid=\"##GUID\">\n" \
                            "    <in method=\"GetDeviceName\"/>\n" \
                            "    <in method=\"GetFirewareVersion\"/>\n" \
                            "    <in method=\"GetKeyDefine\"/>\n" \
                            "    <in method=\"GetPlayStatus\"/>\n" \
                            "    <in method=\"GetSystemVolume\"/>\n" \
                            "    <in method=\"GetBootLogo\"/>\n" \
                            "    <in method=\"GetSensorInfo\"/>\n" \
                            "    <in method=\"GetGPSInfo\"/>\n" \
                            "    <in method=\"GetCurrentLuminance\"/>\n" \
                            "    <in method=\"GetCurrentTemperature\"/>\n" \
                            "    <in method=\"GetCurrentHumity\"/>\n" \
                            "    <in method=\"GetSensorType\"/>\n" \
                            "    <in method=\"GetSwitchTime\"/>\n" \
                            "    <in method=\"GetTimeInfo\"/>\n" \
                            "    <in method=\"GetLuminancePloy\"/>\n" \
                            "    <in method=\"GetScreenInfo\"/>\n" \
                            "    <in method=\"GetLicense\"/>\n" \
                            "    <in method=\"GetEth0Info\">\n" \
                            "        <eth valid=\"true\">\n" \
                            "            <enable value=\"true\"/>\n" \
                            "            <dhcp auto=\"false\"/>\n" \
                            "            <address ip=\"192.168.1.100\" netmask=\"255.255.255.0\" gateway=\"192.168.1.1\" dns=\"192.168.1.1\"/>\n" \
                            "        </eth>\n" \
                            "    </in>\n" \
                            "    <in method=\"GetWifiInfo\"/>\n" \
                            "    <in method=\"GetPppoeInfo\"/>\n" \
                            "    <in method=\"GetDeviceInfo\"/>\n" \
                            "    <in method=\"GetRelay\"/>\n" \
                            "    <in method=\"GetAdminModeInfo\"/>\n" \
                            "</sdk>";

static void ReadData(HSession *currSession, const char *data, huint32 len, void *userData) {
    printf("--------------Read Data:--------------\n");   
    printf("  currSession: %p,\t userData: %p,\t len: %u\n", (void*)currSession, userData, (unsigned int)len);
    printf("  data:        %s\n", data);
    IPData = data; // Store the received data in the global variable
    cout<<"IPData: "<<IPData<<endl;
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

/*
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
*/

// Graceful shutdown function
BOOL WINAPI ConsoleHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_CLOSE_EVENT) {
        g_shutdown = true;
        return TRUE;  // We handled the event
    }
    return FALSE;  // Let other handlers process this event
}

// Custom exception class
// class SocketException : public std::runtime_error {
//     public:
//         SocketException(const std::string& message, int errorCode = 0)
//             : std::runtime_error(message + " (Code: " + std::to_string(errorCode) + ")"),
//             errorCode_(errorCode) {}
        
//         int getErrorCode() const { return errorCode_; }
    
//     private:
//         int errorCode_;
//     };

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
    Exec(core); // Blocks the main thread and waits for Quit to be called from ReadData callback
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
    Exec(core); // Blocks the main thread and waits for Quit to be called from ReadData callback
}

void ScreenFunction(HSession* session){
    cout<<"the screen function to: "<<ScreenFunc<<endl;
    char finalXml[310] = ""; // Ensure it's large enough
    const char* xmlTemplate = XML_SCREEN; // Macro to define the XML template
    snprintf(finalXml, sizeof(finalXml), xmlTemplate,ScreenFunc); // Construct the XML command
    //cout<<"Final XML is: "<<finalXml<<endl;
    if (Connect(session, Device_IP, Device_Port)) {
        // Send the XML command over the session
        if (SendSDK(session, finalXml, strlen(finalXml))) {
            printf("Screen command sent to the display.\n");
        } else {
            printf("Failed to send command.\n");
        }
    } else {
        printf("Connection to the device failed.\n");
    }
    Exec(core); // Blocks the main thread and waits for Quit to be called from ReadData callback
}

void GetIP(HSession* session) {
    char finalXml[500] = ""; // Ensure it's large enough
    const char* xmlTemplate = GET_ETH0_INFO_XML; // Macro to define the XML template
    //snprintf(finalXml, sizeof(finalXml), xmlTemplate); // Construct the XML command
    snprintf(finalXml, sizeof(finalXml), xmlTemplate); // Construct the XML command
    if (Connect(session, Device_IP, Device_Port)) {
        // Send the XML command over the session
        if (SendSDK(session, finalXml, strlen(finalXml))) {
            cout<<finalXml<<endl;
            printf("Got Eth Info Successfully.\n");
         // Call the function to read and process the IP data
        } else {
            printf("Failed to get Eth Info.\n");
        }
    } else {
        printf("Connection to the device failed.\n");
    }
    Exec(core); // Blocks the main thread and waits for Quit to be called from ReadData callback
}

void ReadIPData() {
    cout<<"IPData is: "<<IPData<<endl;
    if (IPData == "") {
        std::cout << "No IP data received!" << std::endl;
  
    }else{
        tinyxml2::XMLDocument doc;
        doc.Parse(IPData.c_str());

        tinyxml2::XMLElement* ethElement = doc.FirstChildElement("sdk")->FirstChildElement("out")->FirstChildElement("eth");
        if (ethElement) {
            const char* ip = ethElement->FirstChildElement("address")->Attribute("ip");
            if (ip) {
                std::cout << "Extracted IP: " << ip << std::endl;
            } else {
                std::cout << "IP address not found!" << std::endl;
            }
        }
        IPData = ""; // Reset IPData to nullptr after processing
    }
} 

void SetIP(HSession* session) {
    char finalXml[500] = ""; // Ensure it's large enough
    const char* xmlTemplate = SET_ETH0_XML;
    snprintf(finalXml, sizeof(finalXml), xmlTemplate, IP2Set);
    // Construct the brightness XML command.
    cout<<"IP changing XML is: "<<finalXml<<endl;
    if (Connect(session, Device_IP, Device_Port)) {
        // Send the XML command over the session
        if (SendSDK(session, finalXml, strlen(finalXml))) {
            printf("IP Updated Successfully to: %s\n ", IP2Set);
        } else {
            printf("Failed to update IP Address the current IP address is %s.\n", Device_IP);
        }
    } else {
        printf("Connection to the device failed.\n");
    }
    Exec(core); // Blocks the main thread and waits for Quit to be called from ReadData callback
}

void GetDeviceInfo(HSession* session ){
    const char* xmlTemplate = GET_DEVICE_INFO;
    cout<<"Retreiving firmware device info,"<<endl;
    if (Connect(session, Device_IP, Device_Port)) {
        // Send the XML command over the session
        if (SendSDK(session, xmlTemplate, strlen(xmlTemplate))) {
            printf("Command sent for retreiving device info sent");
        } else {
            printf("Failed to send command");
        }
    } else {
        printf("Connection to the device failed.\n");
    }
    Exec(core); // Blocks the main thread and waits for Quit to be called from ReadData callback
}

int deserializeJson(string json) {
    int i = 0;
    JsonDocument doc;

    // Attempt to parse the JSON string
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        cout << "deserializeJson() failed: " << error.c_str() << endl;
        return 0;
    }

    // Check if the "ServertoDevice" object exists
    if (!doc.containsKey("ServertoDevice")) {
        cout << "Error: JSON missing 'ServertoDevice' object" << endl;
        return 0;
    }

    JsonObject ServertoDevice = doc["ServertoDevice"];
    
    // Check if required fields exist in ServertoDevice
    if (!ServertoDevice.containsKey("Type")) {
        cout << "Error: JSON missing 'Type' field" << endl;
        return 0;
    }
    Type = ServertoDevice["Type"];

    if (!ServertoDevice.containsKey("Device")) {
        cout << "Error: JSON missing 'Device' field" << endl;
        return 0;
    }
    Device = ServertoDevice["Device"];

    if (!ServertoDevice.containsKey("IP")) {
        cout << "Error: JSON missing 'IP' field" << endl;
        return 0;
    }
    Device_IP = ServertoDevice["IP"];

    if (!ServertoDevice.containsKey("Port")) {
        cout << "Error: JSON missing 'Port' field" << endl;
        return 0;
    }
    Device_Port = ServertoDevice["Port"];

    // Check if Data array exists
    if (!ServertoDevice.containsKey("Data")) {
        cout << "Error: JSON missing 'Data' array" << endl;
        return 0;
    }

    JsonArray dataArray = ServertoDevice["Data"].as<JsonArray>();  // Store the temporary in a variable
    for (JsonObject data : dataArray) // Iterate over the array of objects
    {
        Name = data["Name"];        // "Bay 1", ...   in the case of when configuration is received it will store brightness/power on/ power off.
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
        else if ((strcmp(Type, "Configuration") == 0) && ((strcmp(Name, "Screen") == 0) || (strcmp(Name, "Reboot") == 0))) {
            ScreenFunc = Value; // This will load the value of the screen function to be used in the screen function.
        } 
        else if ((strcmp(Type, "Configuration") == 0) && (strcmp(Name, "GetIP") == 0)){

            continue;
        }        
        else if ((strcmp(Type, "Configuration") == 0) && (strcmp(Name, "SetIP") == 0)){
            IP2Set = Value;            
        } 
        else if ((strcmp(Type, "Configuration") == 0) && (strcmp(Name, "GetFirmwareInfo") == 0)){
            continue;
        
        }else{
            cout<<"Unknown command received..."<<endl;
            continue; // Skip this iteration if the name is not recognized
        }
        i++;
    }       
    return 1;
}


void cleanup_resources(SOCKET sock, HSession* session) {
    // Check if socket is valid before closing
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }
    
    // Clean up Winsock
    WSACleanup();
    
    // Clean up session if it exists
    if (session) {
        Disconnect(session);
        FreeNetSession(session);
    }
    
    // Clean up event core if it exists
    if (core) {
        FreeEventCore(core);
    }
    
    printf("Resources cleaned up successfully\n");
}

int main() {    

    // Set up console handler
    if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
        cerr << "Failed to set control handler!" << endl;
        return -1;
    }

    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in server_addr, client_addr;
    char buffer[2048];
    int addr_len = sizeof(client_addr);
    
    // Create the event core.
    core = CreateEventCore();
    if (!core) {
        cerr << "Failed to create event core!" << endl;
        cleanup_resources(INVALID_SOCKET, nullptr);  // No resources allocated yet
        return -1;
    }
    // Create a network session using the SDK2 protocol.
    HSession *session = CreateNetSession(core, kSDK2);
    if (!session) {
        cerr << "Failed to create network session!" << endl;
        cleanup_resources(INVALID_SOCKET, nullptr);  // Core created but no session
        return -1;
    }

    // Set the callbacks.
    SetNetSession(session, kReadyReadFunc, reinterpret_cast<void *>(ReadData));
    SetNetSession(session, kReadyReadUserData, session);
    SetNetSession(session, kNetStatusFunc, reinterpret_cast<void *>(NetStatus));
    SetNetSession(session, kNetStatusUserData, session);
    SetNetSession(session, kDebugLogFunc, reinterpret_cast<void *>(DebugLog));


    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        cerr << "WSAStartup failed!" << endl;
        cleanup_resources(INVALID_SOCKET, session);  // Core and session created
        return -1;
    }
    
    // Create socket (UDP)
    sock = socket(AF_INET, SOCK_DGRAM, 0);  // UDP socket
    if (sock == INVALID_SOCKET)
    {
        cerr << "Socket creation failed!" << endl;
        cleanup_resources(INVALID_SOCKET, session);  // Winsock initialized but no socket
        return -1;
    }

     // Set socket to non-blocking mode
     u_long mode = 1;  // 1 = non-blocking, 0 = blocking
     if (ioctlsocket(sock, FIONBIO, &mode) != 0) {
         std::cerr << "Failed to set non-blocking mode: " << WSAGetLastError() << std::endl;
         cleanup_resources(INVALID_SOCKET, session);
         return -1;
     }      
    // Set up the server address structure (bind to any available IP address)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);  // The port to listen on
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);  // 0.0.0.0 listens on all available network interfaces

    // Bind the socket to the specified IP address and port
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) 
    {
        cerr << "Bind failed!" << endl;
        closesocket(sock);
        cleanup_resources(sock, session);  // All resources created
        return -1;
    }

    // running tests 
    // cout<<"Display Off"<<endl;
    //  deserializeJson(ConfigPowerOff);
    //  ScreenFunction(session); // Call the ScreenFunction to turn off the display
    //  Sleep(30000);
    //  cout<<"Display On"<<endl;
    // deserializeJson(ConfigPowerOn);
    // ScreenFunction(session); // Call the ScreenFunction to turn on the display
    // Sleep(3000);    
    // cout<<"Changing brightness"<<endl;
    // deserializeJson(gateConfig);
    // HandleBrightness(session); // Call the HandleBrightness function to update the brightness
     cout<<"gantry in"<<endl;
     deserializeJson(gantry_IN);
     Display(session); // Call the Display function to update the display
     Sleep(3000);
    // cout<<"gate in"<<endl;
    // deserializeJson(gate_IN);
    // Display(session); // Call the Display function to update the display
    // Sleep(3000);
    // cout<<"Get IP"<<endl;
    // deserializeJson(GetIPCommand);
    // GetIP(session); // Call the GetIP function to get the IP info
    // ReadIPData(); // Call the getEth0Info function to get the IP info
    // Sleep(3000);
    // cout<<"Reboot"<<endl;
    // deserializeJson(ConfigReboot);
    // ScreenFunction(session); // Call the ScreenFunction to turn off the display
    // Sleep(15000);
    // cout<<"Set IP"<<endl;
    // deserializeJson(SetIPCommand);
    // SetIP(session); // Call the SetIP function to set the IP address
    // Sleep(3000);
    // cout<<"changing brightness"<<endl;
    // deserializeJson(gateConfig1);
    // HandleBrightness(session); // Call the HandleBrightness function to update the brightness
    // Sleep(3000);
    // cout<<"Testing Complete"<<endl;

   
 
    printf("Socket created and bound to %s : %d",SERVER_IP, SERVER_PORT);    

    string lastMessage;  // Store the last received message
   
    while (!g_shutdown) 
    {   
    
        int n = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);     
        if (n > 0){
            buffer[n] = '\0';  // Null-terminate the received string
            cout<<"\nbuffer received: "<<buffer<<endl;
            
            // Check if the new message is different from the last one
            if (buffer != lastMessage) 
            {
                printf("\nReceived new message:%s \n", buffer);      
                deserializeJson(buffer);

                if (deserializeJson(buffer) == 0) {
                    cout << "Failed to deserialize JSON" << endl;
                    continue;  // Skip this iteration if deserialization fails
                }

                if (strcmp(Type, "Command") == 0) {
                    Display(session); // Call the Display function to update the display
                } else if (strcmp(Type, "Configuration") == 0){
                    if (strcmp(Name, "Brightness") == 0) {
                        HandleBrightness(session); // Call the HandleBrightness function to update the brightness
                    } else if((strcmp(ScreenFunc, "Reboot") == 0) || (strcmp(ScreenFunc, "Screen") == 0)) {
                        ScreenFunction(session); // Call the HandlePowerSettings function to update the Power Settings
                    } else if (strcmp(Name, "GetIP") == 0){
                        GetIP(session);
                        ReadIPData(); // Call the getEth0Info function to get the IP info
                        // SetNetSession(session, kReadyReadUserData,reinterpret_cast<void *>(ReadIPData));
                    } else if (strcmp(Name, "SetIP") == 0){
                        SetIP(session);
                    } else if (strcmp(Name, "GetFirmwareInfo") == 0){
                        GetDeviceInfo(session);
                    }
                }
                lastMessage = buffer;  // Update the last received message
            }
            Sleep(1000);
        }
        else if (strlen(buffer) == 0) {
            //printf("Buffer is empty (strlen == 0).\n");
            continue;  // Skip this iteration if the buffer is empty
        } else if (n == SOCKET_ERROR) {
            // Handle the error if needed
            int errorCode = WSAGetLastError();
            printf("recvfrom failed with error code: %d\n", errorCode);
            continue;  // Skip this iteration if recvfrom fails
        } else {
            printf("No new message received.\n");
        }
    }
    
    cout<<"\nExiting the loop"<<endl;
    cleanup_resources(sock, session);

    return 0;
}
