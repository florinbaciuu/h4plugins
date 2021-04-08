/*
 MIT License

Copyright (c) 2020 Phil Bowles <H48266@gmail.com>
   github     https://github.com/philbowles/H4
   blog       https://8266iot.blogspot.com
   groups     https://www.facebook.com/groups/esp8266questions/
              https://www.facebook.com/H4-Esp8266-Firmware-Support-2338535503093896/


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
#pragma once

#include<H4Service.h>

#ifdef ARDUINO_ARCH_ESP8266
    #include<ESP8266WiFi.h>
    #include<ESP8266mDNS.h>
    #include<ESPAsyncTCP.h>
    #include<ESPAsyncUDP.h>
#else
    #include<WiFi.h>
    #include<AsyncTCP.h>
    #include<AsyncUDP.h>
    #include<ESPmDNS.h>
    #include<map> // WHY???
#endif
#include<DNSServer.h>
#include<ArduinoOTA.h>
#include<H4P_SerialCmd.h>
#include<H4P_Signaller.h>
#include<ESPAsyncWebServer.h>
//
#if H4P_USE_WIFI_AP
constexpr const char* goTag(){ return "go"; }
#endif

struct H4P_UI_ITEM { // add title and/or props?
    H4P_UI_TYPE     type;
    H4P_FN_UIGET    f;
    uint8_t         color;
    string          h;
};

using H4P_UI_LIST       = std::map<string,H4P_UI_ITEM>;
class H4P_WiFi: public H4Service, public AsyncWebServer {
#if H4P_USE_WIFI_AP
            DNSServer*          _dns53=nullptr;
#endif
//
            bool                _discoDone;
            uint32_t            _evtID=0;
            AsyncEventSource*   _evts;
            vector<string>      _lines={};
//
                VSCMD(_change);
                VSCMD(_msg);

                string          HAL_WIFI_chipID();
                void            HAL_WIFI_disconnect();
                void            HAL_WIFI_setHost(const string& host);
//
                void            __uiAdd(const string& msg);

        static  String          _aswsReplace(const String& var);
                void            _clear();
                void            _clearUI();
                bool            _cannotConnectSTA(){ 
                    Serial.printf("_cannotConnectSTA() ssid=%s psk=%s can/not=%d\n",CSTR(WiFi.SSID()),CSTR(WiFi.psk()),WiFi.SSID()==h4Tag() || WiFi.psk()==""); 
                    return WiFi.SSID()==h4Tag() || WiFi.psk()=="";
                }
                void            _commonStartup();
                void            _coreStart();
                void            _gotIP();
                void            _lostIP();
                void            _rest(AsyncWebServerRequest *request);
                void            _signalBad(){ h4p[ipTag()]=""; YEVENT(H4PE_SIGNAL,"175,...   ---   ...   "); }
                void            _startWebserver();
                void            _stopWebserver();
        static  void            _wifiEvent(WiFiEvent_t event);
        //      service essentials
    protected:
        virtual void            _handleEvent(const string& s,H4PE_TYPE t,const string& msg) override;
    public:
                void            HAL_WIFI_startSTA(); // Has to be static for bizarre start sequence on ESP32 FFS
//          included here against better wishes due to compiler bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89605
#if H4P_USE_WIFI_AP
                void            _startAP();
        H4P_WiFi(): 
            H4Service(wifiTag(),H4PE_FACTORY | H4PE_GPIO | H4PE_GVCHANGE | H4PE_UIADD | H4PE_UISYNC | H4PE_UIMSG),
            AsyncWebServer(H4P_WEBSERVER_PORT){
                h4p.gvSetInt(goTag(),0,false);
                h4p[ssidTag()]=h4Tag();
                h4p[pskTag()]=h4Tag();
#else
        explicit H4P_WiFi(): H4Service(wifiTag()),AsyncWebServer(H4P_WEBSERVER_PORT){}

        H4P_WiFi(string ssid,string psk,string device=""):
            H4Service(wifiTag(),H4PE_FACTORY | H4PE_GPIO | H4PE_GVCHANGE | H4PE_UIADD | H4PE_UISYNC | H4PE_UIMSG),
            AsyncWebServer(H4P_WEBSERVER_PORT){
                h4p.gvSetstring(ssidTag(),ssid,true);
                h4p.gvSetstring(pskTag(),psk,true);
                h4p.gvSetstring(deviceTag(),device,true);
#endif
                _commonStartup();
            }
                void            change(string ssid,string psk);
#if H4P_LOG_MESSAGES
                void            info() override;
#endif
//
        virtual void            svcDown() override;
        virtual void            svcUp() override;
//
                void            uiAddBoolean(const string& name,const string& section="u"){ _uiAdd(name,H4P_UI_BOOL,section); }
                void            uiAddDropdown(const string& name,H4P_NVP_MAP options,const string& section="u");
                void            uiAddGlobal(const string& name,const string& section="u"){ _uiAdd(name,H4P_UI_TEXT,section); }
                void            uiAddInput(const string& name,const string& section="u");
                void            uiAddText(const string& name,const string& v,const string& section="u"){ _uiAdd(name,H4P_UI_TEXT,section,v); }
                void            uiAddText(const string& name,int v,const string& section="u"){ _uiAdd(name,H4P_UI_TEXT,section,stringFromInt(v)); }

                void            uiSetBoolean(const string& ui,const bool b){ _sendSSE(ui,CSTR(stringFromInt(b))); }
                void            uiSetValue(const string& ui,const int f){ _sendSSE(ui,CSTR(stringFromInt(f))); }
                void            uiSetValue(const string& ui,const string& value){ _sendSSE(ui,CSTR(value)); }
//
                template<typename... Args>
                void            uiMessage(const string& msg, Args... args){ // variadic T<>
                    char* buff=static_cast<char*>(malloc(H4P_REPLY_BUFFER+1));
                    snprintf(buff,H4P_REPLY_BUFFER,CSTR(msg),args...);
                    _sendSSE("",buff);
                    free(buff);
                }
//          syscall only        
        virtual void            _init() override;
                void            _reply(string msg) override { _lines.push_back(msg); }
                void            _sendSSE(const string& name,const string& msg);
                void            _signalOff(){ YEVENT(H4PE_SIGNAL,""); }
                void            _uiAdd(const string& name,H4P_UI_TYPE t,const string& h="u",const string& v="",uint8_t c=H4P_UILED_BI);
};