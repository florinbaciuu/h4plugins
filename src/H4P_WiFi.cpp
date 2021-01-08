/*
 MIT License

Copyright (c) 2020 Phil Bowles <h4plugins@gmail.com>
   github     https://github.com/philbowles/esparto
   blog       https://8266iot.blogspot.com     
   groups     https://www.facebook.com/groups/esp8266questions/
              https://www.facebook.com/Esparto-Esp8266-Firmware-Support-2338535503093896/
                			  

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include<H4PCommon.h>
#include<H4P_WiFi.h>
#include<H4P_SerialCmd.h>
#include<H4P_AsyncWebserver.h>
/*
                                             H / W - D E P E N D E N T   F U N C T I O N S
*/
#ifdef ARDUINO_ARCH_ESP8266
//
//                                                                                                  ESP8266
//
string H4P_WiFi::HAL_WIFI_chipID(){ return stringFromInt(ESP.getChipId(),"%06X"); }

void H4P_WiFi::HAL_WIFI_setHost(const string& host){ WiFi.hostname(CSTR(host)); } // change ????

void H4P_WiFi::HAL_WIFI_startSTA(){
    H4EVENT("HAL_WIFI_startSTA %s %s",CSTR(_cb[ssidTag()]),CSTR(_cb[pskTag()]));
    WiFi.enableAP(false); 
    WiFi.mode(WIFI_STA);
	WiFi.setSleepMode(WIFI_NONE_SLEEP);
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);

    WiFi.begin(CSTR(_cb[ssidTag()]),CSTR(_cb[pskTag()]));
}

void H4P_WiFi::HAL_WIFI_clear(){ } // lose?ESP.eraseConfig(); }

void H4P_WiFi::_stop(){
    H4EVENT("_stop");
    h4.cancelSingleton(H4P_TRID_HOTA);
    WiFi.disconnect(true); // does this actually turn WiFi off?
    ESP.eraseConfig();
}

/*
ESP8266

    WIFI_EVENT_STAMODE_CONNECTED = 0,
    WIFI_EVENT_STAMODE_DISCONNECTED,
    WIFI_EVENT_STAMODE_AUTHMODE_CHANGE,
    WIFI_EVENT_STAMODE_GOT_IP,
    WIFI_EVENT_STAMODE_DHCP_TIMEOUT,
    WIFI_EVENT_SOFTAPMODE_STACONNECTED,
    WIFI_EVENT_SOFTAPMODE_STADISCONNECTED,
    WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED,
    WIFI_EVENT_MAX,
    WIFI_EVENT_ANY = WIFI_EVENT_MAX,
    WIFI_EVENT_MODE_CHANGE

*/
void H4P_WiFi::_wifiEvent(WiFiEvent_t event) {
    switch(event) {
        case WIFI_EVENT_STAMODE_DISCONNECTED:
			h4.queueFunction([](){ h4wifi._lostIP(); });
            break;    
		case WIFI_EVENT_STAMODE_GOT_IP:
			h4.queueFunction([](){ h4wifi._gotIP(); });
			break;
	}
}
#else
//
//                                                                                                                  ESP32
//
string H4P_WiFi::HAL_WIFI_chipID(){
    uint64_t macAddress = ESP.getEfuseMac();
    uint64_t macAddressTrunc = macAddress << 40;
    return stringFromInt(macAddressTrunc >> 40,"%06X");
}

void H4P_WiFi::HAL_WIFI_setHost(const string& host){ WiFi.setHostname(CSTR(host)); }

void H4P_WiFi::HAL_WIFI_startSTA(){
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.enableAP(false); 
	WiFi.setAutoReconnect(true);
    WiFi.begin(CSTR(_cb[ssidTag()]),CSTR(_cb[pskTag()]));
}

void H4P_WiFi::HAL_WIFI_clear(){}

void H4P_WiFi::_stop(){
    h4.cancelSingleton(H4P_TRID_HOTA);
    WiFi.disconnect(true,true);
}
/* ESP32
* WiFi Events
0  SYSTEM_EVENT_WIFI_READY               < ESP32 WiFi ready
1  SYSTEM_EVENT_SCAN_DONE                < ESP32 finish scanning AP
2  SYSTEM_EVENT_STA_START                < ESP32 station start
3  SYSTEM_EVENT_STA_STOP                 < ESP32 station stop
4  SYSTEM_EVENT_STA_CONNECTED            < ESP32 station connected to AP
5  SYSTEM_EVENT_STA_DISCONNECTED         < ESP32 station disconnected from AP
6  SYSTEM_EVENT_STA_AUTHMODE_CHANGE      < the auth mode of AP connected by ESP32 station changed
7  SYSTEM_EVENT_STA_GOT_IP               < ESP32 station got IP from connected AP
8  SYSTEM_EVENT_STA_LOST_IP              < ESP32 station lost IP and the IP is reset to 0
9  SYSTEM_EVENT_STA_WPS_ER_SUCCESS       < ESP32 station wps succeeds in enrollee mode
10 SYSTEM_EVENT_STA_WPS_ER_FAILED        < ESP32 station wps fails in enrollee mode
11 SYSTEM_EVENT_STA_WPS_ER_TIMEOUT       < ESP32 station wps timeout in enrollee mode
12 SYSTEM_EVENT_STA_WPS_ER_PIN           < ESP32 station wps pin code in enrollee mode
13 SYSTEM_EVENT_AP_START                 < ESP32 soft-AP start
14 SYSTEM_EVENT_AP_STOP                  < ESP32 soft-AP stop
15 SYSTEM_EVENT_AP_STACONNECTED          < a station connected to ESP32 soft-AP
16 SYSTEM_EVENT_AP_STADISCONNECTED       < a station disconnected from ESP32 soft-AP
17 SYSTEM_EVENT_AP_STAIPASSIGNED         < ESP32 soft-AP assign an IP to a connected station
18 SYSTEM_EVENT_AP_PROBEREQRECVED        < Receive probe request packet in soft-AP interface
19 SYSTEM_EVENT_GOT_IP6                  < ESP32 station or ap or ethernet interface v6IP addr is preferred
20 SYSTEM_EVENT_ETH_START                < ESP32 ethernet start
21 SYSTEM_EVENT_ETH_STOP                 < ESP32 ethernet stop
22 SYSTEM_EVENT_ETH_CONNECTED            < ESP32 ethernet phy link up
23 SYSTEM_EVENT_ETH_DISCONNECTED         < ESP32 ethernet phy link down
24 SYSTEM_EVENT_ETH_GOT_IP               < ESP32 ethernet got IP from connected AP
25 SYSTEM_EVENT_MAX

    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6

*/

void H4P_WiFi::_wifiEvent(WiFiEvent_t event) {
    switch(event) {
        case SYSTEM_EVENT_STA_LOST_IP:
//            Serial.printf("32 EVENT SYSTEM_EVENT_STA_LOST_IP\n");
			h4.queueFunction([](){ h4wifi._lostIP(); }); // ? if?
            break;
		case SYSTEM_EVENT_STA_GOT_IP:
//            Serial.printf("32323232 EVENT SYSTEM_EVENT_STA_GOT_IP\n");
			h4.queueFunction([](){ h4wifi._gotIP(); });
			break;
//        default:
//            Serial.printf("32323232 FNOZE %d\n",event);
//            break;
	}
}
#endif
/*
                                                                                                 C O M M O N   F U N C T I O N S
*/
void H4P_WiFi::clear(){ 
    H4EVENT("clear()");
	stop();
    HAL_FS.remove(CSTR(string("/"+string(deviceTag()))));
    WiFi.mode(WIFI_OFF); // cos i dont believe disco true works
    HAL_WIFI_clear();
}

void H4P_WiFi::_start(){
    H4EVENT("_mcuStart mode=%d",WiFi.getMode());
    if(WiFi.getMode()!=WIFI_STA || WiFi.SSID()=="") HAL_WIFI_startSTA();
}

uint32_t H4P_WiFi::_change(vector<string> vs){ return guardString2(vs,[this](string a,string b){ change(a,b); return H4_CMD_OK; }); }

bool H4P_WiFi::_getPersistentValue(string v,string prefix){
    string persistent=replaceAll(H4P_SerialCmd::read("/"+v),"\r\n","");
    _cb[v]=persistent.size() ? persistent:string(prefix)+_cb[chipTag()];
    return persistent.size();
}

STAG(tcp)

void H4P_WiFi::_gotIP(){
    _discoDone=false;
    _cb[ipTag()]=WiFi.localIP().toString().c_str();
    _cb[ssidTag()]=CSTR(WiFi.SSID());
    _cb[pskTag()]=CSTR(WiFi.psk());

    string host=_cb[deviceTag()];
#ifdef H4P_USE_OTA
    h4.every(H4WF_OTA_RATE,[](){ ArduinoOTA.handle(); },nullptr,H4P_TRID_HOTA,true);
    HAL_WIFI_setHost(host);
    if(MDNS.begin(CSTR(host))) {
        MDNS.addService(h4Tag(),tcpTag(),666);
        MDNS.addServiceTxt(h4Tag(),tcpTag(),"id",CSTR(_cb[chipTag()]));
        MDNS.addServiceTxt(h4Tag(),tcpTag(),ipTag(),CSTR(_cb[ipTag()]));
    } //else Serial.println("Error starting mDNS");
  	ArduinoOTA.setHostname(CSTR(host));
	ArduinoOTA.setRebootOnSuccess(false);	
	ArduinoOTA.begin(); // matching end???
#endif
    H4EVENT("IP=%s",CSTR(_cb[ipTag()]));
    _upHooks();
}

void H4P_WiFi::_hookIn(){
    _cb[chipTag()]=HAL_WIFI_chipID();
    _cb[boardTag()]=replaceAll(H4_BOARD,"ESP8266_","");
    if(!_getPersistentValue(deviceTag(),"H4-")) if(_device!="") _cb[deviceTag()]=_device;
    H4EVENT("Device %s chip %s",CSTR(_cb[deviceTag()]),CSTR(_cb[chipTag()]));
    _getPersistentValue(h4UIvTag(),"NO UI! ");
    WiFi.persistent(true);
    WiFi.onEvent(_wifiEvent);

    h4asws._uiAdd(4,boardTag(),H4P_UI_LABEL);
    h4asws._uiAdd(5,chipTag(),H4P_UI_LABEL);
    h4asws._uiAdd(1,deviceTag(),H4P_UI_LABEL);
    h4asws._uiAdd(6,ipTag(),H4P_UI_LABEL,"",[]{ return _cb[ipTag()]; }); // cos we don't know it yet
    h4asws._uiAdd(7,"H4v",H4P_UI_LABEL,H4_VERSION);
    h4asws._uiAdd(8,h4PvTag(),H4P_UI_LABEL);
    h4asws._uiAdd(9,h4UIvTag(),H4P_UI_LABEL);
    if(isLoaded(mqttTag())) h4asws._uiAdd(10,"Pangolin Vn",H4P_UI_LABEL,_cb[pmvTag()]);
    if(isLoaded(upnpTag())) {
        h4asws._uiAdd(2,"name",H4P_UI_LABEL,"",[]{ return _cb[nameTag()]; }); // cos we don't know it yet
        h4cmd.addCmd("host2",_subCmd,0,CMDVS(_host2));
    }
}

uint32_t H4P_WiFi::_host(vector<string> vs){
    return guard1(vs,[this](vector<string> vs){
        return ([this](string h){
            host(h); 
            return H4_CMD_OK;
        })(H4PAYLOAD);
    });
}

uint32_t H4P_WiFi::_host2(vector<string> vs){ return guardString2(vs,[this](string a,string b){ setBothNames(a,b); return H4_CMD_OK; }); }

void H4P_WiFi::_lostIP(){
    H4EVENT("_lostIP() done=%d",_discoDone);
    h4.cancelSingleton(H4P_TRID_HOTA);
    if(!_discoDone){
        _downHooks();
        _discoDone=true;
    }
}

void H4P_WiFi::_setPersistentValue(string n,string v,bool reboot){
    if(_cb[n]!=v){
        H4P_SerialCmd::write("/"+n,v);
        if(reboot) h4reboot();
    }
}

void H4P_WiFi::change(string ssid,string psk){ // add device / name?
    stop();
    _cb[ssidTag()]=ssid;
    _cb[pskTag()]=psk;
    HAL_WIFI_startSTA();
    h4cmd.showFS();
    h4reboot();
}

void H4P_WiFi::setBothNames(const string& hostname,const string& friendly){ 
    H4EVENT("setBothNames %s %s",CSTR(hostname),CSTR(friendly))
    if(isLoaded(upnpTag())) _setPersistentValue(nameTag(),friendly,false);
    host(hostname);
}
