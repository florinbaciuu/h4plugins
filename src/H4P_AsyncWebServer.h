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
#ifndef H4P_AsyncWebServer_HO
#define H4P_AsyncWebServer_HO

#include<H4PCommon.h>
#include<H4P_WiFi.h>
#include<H4P_BinaryThing.h>
#include<ESPAsyncWebServer.h>

#define H4P_ASWS_EVT_TIMEOUT    30000

extern void onUiChange(const string& name,const string& value);

enum H4P_UI_TYPE {
    H4P_UI_LABEL,
    H4P_UI_TEXT,
    H4P_UI_BOOL,
    H4P_UI_ONOF,
    H4P_UI_INPUT,
    H4P_UI_DROPDOWN
};

using H4P_FN_UITXT      = function<string(void)>;
using H4P_FN_UINUM      = function<int(void)>;
using H4P_FN_UIBOOL     = function<boolean(void)>;
using H4P_FN_UICHANGE   = function<void(const string&)>;

struct H4P_UI_ITEM {    
    string          id;
    H4P_UI_TYPE     type;
    string          value;
    H4P_FN_UITXT    f;
    H4P_FN_UICHANGE c;
};

using H4P_UI_LIST       = std::map<int,H4P_UI_ITEM>;

class H4P_AsyncWebServer: public AsyncWebServer, public H4Plugin {
            uint32_t            _seq=100;
            uint32_t            _evtID=0;
            H4P_BinaryThing*    _btp=nullptr;
            AsyncEventSource*   _evts;
            H4_CMD_MAP          _local={};
            H4P_UI_LIST         _userItems={};
            vector<string>      _lines={};
            H4_FN_VOID          _onC,_onD;

            H4_CMD_ERROR        __uichgCore(const string& a,const string& b,H4P_FN_UICHANGE f);

            VSCMD(_gpio);
            VSCMD(_msg);
            VSCMD(_uichg);

                void            _rest(AsyncWebServerRequest *request);
//      service essentials
                void            _start() override;
                void            _stop() override;
                void            _hookIn() override;
                void            _greenLight() override {};
    public:
        H4P_AsyncWebServer(H4_FN_VOID onClientConnect=nullptr,H4_FN_VOID onClientDisconnect=nullptr): AsyncWebServer(80),H4Plugin(aswsTag()){
            _onC=onClientConnect;
            _onD=onClientDisconnect;

            _cmds={
                {_pName,    { H4PC_H4, _subCmd, nullptr}},
                {"msg",     { _subCmd, 0, CMDVS(_msg)}},
                {"uichg",   { _subCmd, 0, CMDVS(_uichg)}}
            };
        }
        static  String          aswsReplace(const String& var);
                void            show() override;
                void            uiAddLabel(const string& name){ _uiAdd(_seq++,name,H4P_UI_LABEL,_cb[name]); }
                void            uiAddLabel(const string& name,const string& v){ _uiAdd(_seq++,name,H4P_UI_LABEL,v); }
                void            uiAddLabel(const string& name,const int v){ _uiAdd(_seq++,name,H4P_UI_LABEL,stringFromInt(v)); }
                void            uiAddLabel(const string& name,H4P_FN_UITXT f){ _uiAdd(_seq++,name,H4P_UI_LABEL,"",f); }
                void            uiAddLabel(const string& name,H4P_FN_UINUM f){ _uiAdd(_seq++,name,H4P_UI_LABEL,"",[f]{ return stringFromInt(f()); }); }
                void            uiAddGPIO();
                H4_CMD_ERROR    uiAddGPIO(uint8_t pin);
                void            uiAddBoolean(const string& name,const boolean tf,H4P_FN_UICHANGE a=nullptr){ _uiAdd(_seq++,name,H4P_UI_BOOL,"",[tf]{ return tf ? "1":"0"; },a); }
                void            uiAddBoolean(const string& name,H4P_FN_UIBOOL f,H4P_FN_UICHANGE a=nullptr){ _uiAdd(_seq++,name,H4P_UI_BOOL,"",[f]{ return f() ? "1":"0"; },a); }

                void            uiAddDropdown(const string& name,H4P_CONFIG_BLOCK options,H4P_FN_UICHANGE onChange=nullptr);

                void            uiAddInput(const string& name,H4P_FN_UITXT f=nullptr,H4P_FN_UICHANGE onChange=nullptr);
                void            uiAddInput(const string& name,const string& value,H4P_FN_UICHANGE onChange=nullptr);

                void            uiSetInput(const string& name,const string& value){ _sendSSE(CSTR(name),CSTR(value)); }
                void            uiSetBoolean(const string& name,const bool b){ _sendSSE(CSTR(name),CSTR(stringFromInt(b))); }
                void            uiSetLabel(const string& name,const int f){ _sendSSE(CSTR(name),CSTR(stringFromInt(f))); }
                void            uiSetLabel(const string& name,const string& value){ _sendSSE(CSTR(name),CSTR(value)); }

                void            uiSync();
//
                template<typename... Args>
                void            uiMessage(const string& msg, Args... args){ // variadic T<>
                    char* buff=static_cast<char*>(malloc(H4P_REPLY_BUFFER+1));
                    snprintf(buff,H4P_REPLY_BUFFER,CSTR(msg),args...);
                    _sendSSE(NULL,buff);
                    free(buff);
                }
//          syscall only
                void            _reply(string msg) override { _lines.push_back(msg); }
                void            _sendSSE(const char* name,const char* msg);
                void            _uiAdd(int seq,const string& i,H4P_UI_TYPE t,const string& v="",H4P_FN_UITXT f=nullptr,H4P_FN_UICHANGE a=nullptr);
};

extern __attribute__((weak)) H4P_AsyncWebServer h4asws;

#endif // H4P_AsyncWebServer_H