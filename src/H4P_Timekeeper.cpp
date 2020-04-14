
/*
 MIT License

Copyright (c) 2019 Phil Bowles <h4plugins@gmail.com>
   github     https://github.com/philbowles/H4P_Timekeeper
   blog       https://8266iot.blogspot.com     
   groups     https://www.facebook.com/groups/esp8266questions/
              https://www.facebook.com/H4P_Timekeeper-Esp8266-Firmware-Support-2338535503093896/
                			  

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
#ifdef ARDUINO_ARCH_ESP8266
#include<H4P_Timekeeper.h>
#include<H4P_SerialCmd.h>
#include<H4P_BinaryThing.h>

void __attribute__((weak)) onRTC(){}

constexpr uint32_t secsInDay(){ return 86400; }
constexpr uint32_t msInDay(){ return 1000*secsInDay(); }

H4P_Timekeeper::H4P_Timekeeper(const string& ntp1,const string& ntp2,int tzo): H4Plugin(timeTag()){
    _cmds={
        {_pName,   { H4PC_H4, _subCmd, nullptr}}, // root for this plugin, e.g. h4/ME...
        {"change", { _subCmd,       0, CMDVS(_change)}},
        {"sync",   { _subCmd,       0, CMD(sync)}},
        {"tz",     { _subCmd,       0, CMDVS(_tz)}}
    };
    _setupSNTP(ntp1,ntp2);
    sntp_set_timezone(tzo);
}

uint32_t H4P_Timekeeper::__alarmCore (vector<string> vs,bool daily,H4BS_FN_SWITCH f){
    vector<string> vg=split(H4PAYLOAD,",");
    if(vg.size()>2) return H4_CMD_TOO_MANY_PARAMS;
    if(vg.size()<2) return H4_CMD_TOO_FEW_PARAMS;
    int T=parseTime(vg[0]);
    if(T<0) return H4_CMD_PAYLOAD_FORMAT;
    string b=vg[1];
    if(!isNumeric(b)) return H4_CMD_NOT_NUMERIC;
    int onoff=atoi(CSTR(b));
    if(_mss00){ // onRTC
        H4EVENT("%s %s -> %s",daily ? "Daily":"S/Shot",CSTR(strTime(T)),onoff ? "ON":"OFF");
        int msDue=T-msSinceMidnight();
        if(msDue < 0) msDue+=msInDay();
        uint32_t u=daily ? H4P_TRID_DALY:H4P_TRID_SHOT;
        h4.add([this,f,onoff]{
            if(_mss00) f(onoff);
            else H4EVENT("RTC ignored"); // don't fire if NTP not sync'ed 
        },msDue,daily ? msDue:0,H4Countdown(1),nullptr,TAG(daily ? 7:3));
        return H4_CMD_OK;
    } else H4EVENT("Alarm ignored"); // don't set if NTP not sync'ed 
}

uint32_t H4P_Timekeeper::_at(vector<string> vs){ return __alarmCore(vs,false,[this](bool b){ _btp->turn(b); }); }

uint32_t H4P_Timekeeper::_change(vector<string> vs){ return guardString2(vs,[this](string a,string b){ change(a,b); }); }

uint32_t H4P_Timekeeper::_daily(vector<string> vs){ return __alarmCore(vs,true,[this](bool b){ _btp->turn(b); }); }

void H4P_Timekeeper::_hookIn(){ 
    DEPEND(wifi);
    H4Plugin* p=isLoaded(onofTag());
    if(p) {
        _btp=reinterpret_cast<H4P_BinaryThing*>(p);
        h4cmd.addCmd("at",_subCmd,0,CMDVS(_at));
        h4cmd.addCmd("daily",_subCmd,0,CMDVS(_daily));
    }
}

void H4P_Timekeeper::_setupSNTP(const string& ntp1, const string& ntp2){
    _ntp1=ntp1;
    _ntp2=ntp2;
    sntp_setservername(0,CSTR(_ntp1));
    sntp_setservername(1,CSTR(_ntp2));
}

void H4P_Timekeeper::_start(){
    sntp_init();
    if(!_mss00){
        h4.repeatWhile(
            [this]{ return !_mss00; },
            H4P_TIME_HOLDOFF,
            [this]{ sync(); },
            [this]{
                static bool gotRTC=false; 
                H4EVENT("%u GOT NTP %s",millis(),CSTR(clockTime()));
                if(!gotRTC) {
                    onRTC();
                    gotRTC=true;
                    h4.every(H4P_TIME_RESYNC,[this]{ sync(); },nullptr,H4P_TRID_SYNC,true); // tweakables
                }
            },
            H4P_TRID_TIME,
            true
        );
    }
}

void H4P_Timekeeper::_stop(){ 
    h4.cancelSingleton({H4P_TRID_TIME,H4P_TRID_SYNC});
	sntp_stop();
}

uint32_t H4P_Timekeeper::_tz(vector<string> vs){
    return guardInt1(vs,[this](int v){
        tz(v); 
        return H4_CMD_OK;
    });
}
//
//
//
void H4P_Timekeeper::at(const string& when,bool onoff,H4BS_FN_SWITCH f){ __alarmCore({when+","+stringFromInt(onoff)},false,f); }

void H4P_Timekeeper::atSource(const string& when,bool onoff){ if(_btp) _at({when+","+stringFromInt(onoff)}); }

void H4P_Timekeeper::change(const string& ntp1,const string& ntp2){ _setupSNTP(ntp1,ntp2); }

void H4P_Timekeeper::daily(const string& when,bool onoff,H4BS_FN_SWITCH f){ __alarmCore({when+","+stringFromInt(onoff)},true,f); }

void H4P_Timekeeper::dailySource(const string& when,bool onoff){ if(_btp) _daily({when+","+stringFromInt(onoff)}); }

string H4P_Timekeeper::minutesFromNow(uint32_t m){
    string longtime=h4tk.strTime((msSinceMidnight())+(m*60000));  
    return string(longtime.begin(),longtime.end()-3); 
}

int H4P_Timekeeper::parseTime(const string& ts){ // in milliseconds!
	vector<string> parts=split(ts,":");
    uint32_t    h,m,s=0;
    switch(parts.size()){
        case 3:
	        s=atoi(CSTR(parts[2]));
            if(s>59) return -1;
        case 2:
            m=atoi(CSTR(parts[1]));
            if(m>59) return -1;
            h=atoi(CSTR(parts[0]));
            if(h>23) return -1;
            break;
        default:
            return -1;
    }
	return 1000*(s+(m*60) + (h*3600));
}

void H4P_Timekeeper::setSchedule(H4P_SCHEDULE shed,H4BS_FN_SWITCH f){
  for(auto s:shed){
    h4tk.at(s.first,ON,f);
    h4tk.at(s.second,OFF,f);    
  }
}
// optimise with _btpON lambda?
void H4P_Timekeeper::setScheduleSource(H4P_SCHEDULE shed){
  for(auto s:shed){
    h4tk.atSource(s.first,ON);
    h4tk.atSource(s.second,OFF);
  }
}

void H4P_Timekeeper::show(){
//    reply("_mss00=%d, ms since 00:00=%d",_mss00,msSinceMidnight());
    reply("TZO=%d",sntp_get_timezone());
    reply("%s %s",CSTR(_ntp1),CSTR(_ntp2));
    reply("Wallclock=%s, UpTime=%s",CSTR(clockTime()),CSTR(upTime()));
}

string H4P_Timekeeper::strTime(uint32_t t){ // milliseconds!
	char buf[9];
    uint32_t sex=t/1000;
	sprintf(buf,"%02d:%02d:%02d",(sex%secsInDay())/3600,(sex/60)%60,sex%60);
	return string(buf);
}

void H4P_Timekeeper::sync(){
	long stamp=sntp_get_current_timestamp();
	if(stamp > 30000){ // 28800 +leeway: default is GMT+8
		H4EVENT("GOOD STAMP %u time %lu gtz=%d grt %s",millis(),stamp,sntp_get_timezone(),sntp_get_real_time(stamp));
		vector<string> dp=split(sntp_get_real_time(stamp)," ");
        _mss00=parseTime(dp[3])-millis();
        H4EVENT("%s _mss00=%d",CSTR(dp[3]),_mss00);
	} else H4EVENT("%u NO TIME AT ALL",millis());
}

void H4P_Timekeeper::tz(uint32_t tzOffset){
    _stop();
    sntp_set_timezone(tzOffset);
    _mss00=0; // reset timekeeping, force sync
    _start();
}

string H4P_Timekeeper::upTime(){
	uint32_t t=millis();
	return stringFromInt(t / msInDay(),"%02d:")+strTime(t);
}

#endif // esp8266 only