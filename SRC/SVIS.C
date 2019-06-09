// ALVIS - Allegro Visualization Plugin
//
// With thanks to : 
// Justin Frankel, Ryan Geiss and Francis Gastellu / Nullsoft.
// all Allegro programmers.
//
// Programmed by Karthik "Guilt" Kumar V for use with Allegro
// Co-Programmed by Nikhil Karnad
//
// Added support for WA5. WA5 Rocks.

#include "allegro.h"
#include "winalleg.h"
#include "wa_ipc.h"
#include "stdio.h"
#include "vis.h"
#include "fft.c"

//Clipping Values
#define MAXXVAL 4000
#define MAXYVAL 3000
#define OBRUSH BLACK_BRUSH

#define ALVER ALLEGRO_VERSION_STR

#define SZLEN 128 // Must be enuff .

#ifndef ALLEGRO_MSVC

// GFX Driver Support
BEGIN_GFX_DRIVER_LIST
END_GFX_DRIVER_LIST

// Joystick Support
BEGIN_JOYSTICK_DRIVER_LIST
END_JOYSTICK_DRIVER_LIST

// MIDI Support
BEGIN_MIDI_DRIVER_LIST
END_MIDI_DRIVER_LIST

// Sound Support
BEGIN_DIGI_DRIVER_LIST
END_DIGI_DRIVER_LIST

#endif

// Window Flags .
int FullyExited=1;
int btinited=0;
int alinited=0,alerr=0;
int BPP=0;

// Allegro stuff .
BITMAP *usedbuf=NULL;
BITMAP *boombox=NULL;

//Boom Box Stuff .
int boom_flag=1, boom_what=1;
char boom_file[MAX_PATH];
unsigned char *boom_data[2];

// For OSD .
char szSongTitleOri[SZLEN],szSongTitle[SZLEN],
     szSongPos[SZLEN],szSongLen[SZLEN],szTotStr[3*SZLEN];
char szSongFormat[SZLEN]={" *** "};
float songpos,songlen;
int SPRLEN=0;

// For Windowing .
int WIDTH=0,HEIGHT=0;

// INI.
char szPFName[]="alvis.ini";

//Boom Box BMP File.
char szBBPFName[]="boom.bmp";

// title etc.
char 
szTitleStuff[]="Allegro Visualization Plugin",
szNotAvail[]="Feature not available",
szAbout[]= "                    Copyrights © 2004 ODE    \n"
           "                      By Karthik Kumar V     \n"
           "                              and            \n"
           "                        Nikhil Karnad        \n"
           "                                             \n"
           "                 Based on Vis_MiniSDK v1.01  \n"
           "                       By Justin Frankel     \n"
           "                                             \n"
           "                 Based on Vis_MegaSDK v1.03  \n"
           "                          By Ryan Geiss      \n"
           "                                             \n"
           "                     Based on wa502_sdk      \n"
           "                     By Francis Gastellu     \n"           
           "                                             \n"
           "                   Based on Allegro v"ALVER" \n"
#ifndef ALLEGRO_STATICLINK
           "                                             \n"
           "           Requires Allegro v"ALVER" Runtime \n"
#endif
           "                                             \n",
szAppName[] = "AlVis",
szSx[]="Screen_x",
szSy[]="Screen_y",
szStats[]="OSD",
szBoomBG[]="BG",
szBoomWhat[]="BBMode";

// configuration declarations
int config_x=0, config_y=0, stats=1;    // screen X position ,Y position and Statistics Flag repsectively

// returns a winampVisModule when requested. Used in hdr, below
winampVisModule *getModule(int which);

//For WA5 Support.
embedWindowState w;
int wa5flag=0;

// "member" functions
void analyze(struct winampVisModule *this_mod); // analysis
void config(struct winampVisModule *this_mod);  // configuration dialog
void nofeat();                                  // no feature dialog
int  init(struct winampVisModule *this_mod);    // initialization for module
int  render1(struct winampVisModule *this_mod); // rendering for module 1
int  render2(struct winampVisModule *this_mod); // rendering for module 2
int  render3(struct winampVisModule *this_mod); // rendering for module 3
int  render4(struct winampVisModule *this_mod); // rendering for module 4
int  render5(struct winampVisModule *this_mod); // rendering for module 5
void quit(struct winampVisModule *this_mod);    // deinitialization for module

int fraps[2]; //Frame Parameters

/* Nullsoft Code for Sound Analysis */
float temp_wave[2][576];
float m_used    = 0.5f;   // A Low value yields better results
float bt_tm_dec = 1.0f; // Sum Decayer
int min_infi=300;

struct
{
    int   m_is_beat;
    int   timer;
    float m_has_fallen;
    float m_last_hit;
    float m_thresh;
    float m_limit;
    float sum[3];            // bass, mids, treble 
    float infi_avg[3];       // bass, mids, treble 
    float imm[3];            // bass, mids, treble, no damping, for left channel (long-term average is 1)
    float avg[3];            // bass, mids, treble, some damping, for left channel (long-term average is 1)
    float med_avg[3];        // bass, mids, treble, more damping, for left channel (long-term average is 1)
    float long_avg[3];       // bass, mids, treble, heavy damping, for left channel (long-term average is 1)
    float fWaveform[576];    // 576 samples for each channel
    float fSpectrum[NFREQ];  // NFREQ frequencies for each channel
}td_soundinfo[2];
/* Nullsoft Code for Sound Analysis */

// first module
winampVisModule mod1 =
{
    "WaveForm Analyser",
    NULL,   // hwndParent
    NULL,   // hDllInstance
    0,      // sRate
    0,      // nCh
    75,     // latencyMS
    25,     // delayMS
    0,      // spectrumNch
    2,      // waveformNch
    { {0},{0} },    // spectrumData
    { {0},{0} },    // waveformData
    config,
    init,
    render1, 
    quit
};

// second module
winampVisModule mod2 =
{
    "Spectrum Analyser",
    NULL,   // hwndParent
    NULL,   // hDllInstance
    0,      // sRate
    0,      // nCh
    175,    // latencyMS
    25,     // delayMS
    2,      // spectrumNch
    0,      // waveformNch
    { {0},{0} },    // spectrumData
    { {0},{0} },    // waveformData
    config,
    init,
    render2, 
    quit
};

// third module
winampVisModule mod3 =
{
    "VU Analyser",
    NULL,   // hwndParent
    NULL,   // hDllInstance
    0,      // sRate
    0,      // nCh
    75,     // latencyMS
    25,     // delayMS
    0,      // spectrumNch
    2,      // waveformNch
    { {0},{0} },    // spectrumData
    { {0},{0} },    // waveformData
    config,
    init,
    render3, 
    quit
};

// fourth module
winampVisModule mod4 =
{
    "The Grind",
    NULL,   // hwndParent
    NULL,   // hDllInstance
    0,      // sRate
    0,      // nCh
    105,    // latencyMS
    5,      // delayMS
    0,      // spectrumNch
    2,      // waveformNch
    { {0},{0} },    // spectrumData
    { {0},{0} },    // waveformData
    config,
    init,
    render4, 
    quit
};

// fifth module
winampVisModule mod5 =
{
    "Boom Box - Explosion",
    NULL,   // hwndParent
    NULL,   // hDllInstance
    0,      // sRate
    0,      // nCh
    105,    // latencyMS
    5,      // delayMS
    2,      // spectrumNch
    2,      // waveformNch
    { {0},{0} },    // spectrumData
    { {0},{0} },    // waveformData
    config,
    init,
    render5, 
    quit
};

// our window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
HWND hMainWnd; // main window handle

// Module header, includes version, description, and address of the module retriever function
winampVisHeader hdr = { VIS_HDRVER, szTitleStuff, getModule };

// this is the only exported symbol. returns our main header.
// if you are compiling C++, the extern "C" { is necessary, so we just #ifdef it
#ifdef __cplusplus
extern "C" {
#endif
__declspec( dllexport ) winampVisHeader *winampVisGetHeader()
{
    return &hdr;
}
#ifdef __cplusplus
}
#endif

void config_getinifn(struct winampVisModule *this_mod, char *ini_file)
{   // makes a .ini file in the winamp directory named "alvis.ini"
    char *p;
    GetModuleFileName(this_mod->hDllInstance,ini_file,MAX_PATH);
    p=ini_file+strlen(ini_file);
    while (p >= ini_file && *p != '\\') p--;
    if (++p >= ini_file) *p = 0;
    strcat(ini_file,szPFName);
}

void config_read(struct winampVisModule *this_mod)
{
    char ini_file[MAX_PATH];
    config_getinifn(this_mod,ini_file);
    config_x = GetPrivateProfileInt(this_mod->description,szSx,config_x,ini_file);
    config_y = GetPrivateProfileInt(this_mod->description,szSy,config_y,ini_file);
    stats = GetPrivateProfileInt(this_mod->description,szStats,stats,ini_file);
    if(this_mod==&mod5)
    {
    boom_what = GetPrivateProfileInt(this_mod->description,szBoomWhat,boom_what,ini_file);
    boom_flag = GetPrivateProfileInt(this_mod->description,szBoomBG,boom_flag,ini_file);
    }

    if(config_x<0||config_x>MAXXVAL) config_x=0;
    if(config_y<0||config_y>MAXYVAL) config_y=0;
    if(stats<0||stats>1){stats=1;}

    if(this_mod==&mod5)
    {
    if(boom_what<0||boom_what>1){boom_what=1;}
    if(boom_flag<0||boom_flag>1){boom_flag=1;}
    }
}

void config_write(struct winampVisModule *this_mod)
{
    char string[32];
    char ini_file[MAX_PATH];

    config_getinifn(this_mod,ini_file);

    if(config_x<0||config_x>MAXXVAL) config_x=0;
    if(config_y<0||config_y>MAXYVAL) config_y=0;
    if(stats<0||stats>1){stats=1;}

    if(this_mod==&mod5)
    {
    if(boom_what<0||boom_what>1){boom_what=1;}
    if(boom_flag<0||boom_flag>1){boom_flag=1;}
    }

    wsprintf(string,"%d",config_x);
    WritePrivateProfileString(this_mod->description,szSx,string,ini_file);
    wsprintf(string,"%d",config_y);
    WritePrivateProfileString(this_mod->description,szSy,string,ini_file);
    wsprintf(string,"%d",stats);
    WritePrivateProfileString(this_mod->description,szStats,string,ini_file);

    if(this_mod==&mod5)
    {
    wsprintf(string,"%d",boom_what);
    WritePrivateProfileString(this_mod->description,szBoomWhat,string,ini_file);
    wsprintf(string,"%d",boom_flag);
    WritePrivateProfileString(this_mod->description,szBoomBG,string,ini_file);
    }

}

// Generally Funky!
void makescroll(char src[SZLEN],char dest[SZLEN],int gt,int wndsize)
{
    char temp[SZLEN*3];
    int len=0,j=0,strt;
    for(;src[len];++len);
    *dest=0;
    strcpy(temp,src);
    strcat(temp,szSongFormat);
    strcat(temp,src);
    if(len<=0||wndsize<=0) return;
    strt=gt%len;
    for(;j<wndsize;++j)
        dest[j]=temp[strt+j];
    dest[j]=0;
}

/* Following Code from Vis_MegaSDK v1.03 */

// get Song Title
void GetWinampSongTitle(HWND hWndWinamp, char *szSongTitle, int nSize)
{
    char *p;
    int pos;

    szSongTitle[0] = 0;

    if (GetWindowText(hWndWinamp, szSongTitle, nSize))
    {
        // remove ' - Winamp' if found at end
        if (strlen(szSongTitle) > 9)
        {
            int check_pos = strlen(szSongTitle) - 9;
            if (lstrcmp(" - Winamp", (char *)(szSongTitle + check_pos)) == 0)
                szSongTitle[check_pos] = 0;
        }

        // remove ' - Winamp [Paused]' if found at end
        if (strlen(szSongTitle) > 18)
        {
            int check_pos = strlen(szSongTitle) - 18;
            if (lstrcmp(" - Winamp [Paused]", (char *)(szSongTitle + check_pos)) == 0)
                szSongTitle[check_pos] = 0;
        }

        // remove ' - Winamp [Stopped]' if found at end
        if (strlen(szSongTitle) > 19)
        {
            int check_pos = strlen(szSongTitle) - 19;
            if (lstrcmp(" - Winamp [Stopped]", (char *)(szSongTitle + check_pos)) == 0)
                szSongTitle[check_pos] = 0;
        }
        
        // remove song # and period from beginning
        p = szSongTitle;
        while (*p >= '0' && *p <= '9') p++;
        if (*p == '.' && *(p+1) == ' ')
        {
            p += 2;
            pos = 0;
            while (*p != 0)
            {
                szSongTitle[pos++] = *p;
                p++;
            }
            szSongTitle[pos++] = 0;
        }

        {
            int pos = 0,x;
            int len = strlen(szSongTitle);
            while (szSongTitle[pos])
            {
                if (szSongTitle[pos] == '&')
                {
                    for (x=len; x>=pos; x--)
                        szSongTitle[x+1] = szSongTitle[x];
                    len++;
                    pos++;
                }
                pos++;
            }
        }
    }
}

// get Song Position String
void GetWinampSongPosAsText(HWND hWndWinamp, char *szSongPos)
{
    int nSongPosMS,minutes,seconds,dsec;
    float time_s;
    // note: size(szSongPos[]) must be at least 64.
    szSongPos[0] = 0;
    nSongPosMS = SendMessage(hWndWinamp,WM_USER,0,105);
    if (nSongPosMS > 0)
    {
        time_s = nSongPosMS*0.001f;
        minutes = (int)(time_s/60);
        time_s -= minutes*60;
        seconds = (int)time_s;
        time_s -= seconds;
        dsec = (int)(time_s*100);
        sprintf(szSongPos, "%d:%02d.%02d", minutes, seconds, dsec);
    }    
}

// get Song Length String
void GetWinampSongLenAsText(HWND hWndWinamp, char *szSongLen)
{
    int nSongLenMS;
    // note: size(szSongLen[]) must be at least 64.
    szSongLen[0] = 0;
    nSongLenMS = SendMessage(hWndWinamp,WM_USER,1,105)*1000;
    if (nSongLenMS > 0)
    {
        int len_s = nSongLenMS/1000;
        int minutes = len_s/60;
        int seconds = len_s - minutes*60;
        sprintf(szSongLen, "%d:%02d", minutes, seconds);
    }    
}

// get Song Position
float GetWinampSongPos(HWND hWndWinamp)
{
    // returns answer in seconds
    return (float)SendMessage(hWndWinamp,WM_USER,0,105)*0.001f;
}

// get Song Length
float GetWinampSongLen(HWND hWndWinamp)
{
    // returns answer in seconds
    return (float)SendMessage(hWndWinamp,WM_USER,1,105);
}

// Plugin Stuff
int WaitUntilPluginFinished()
{
    int slept = 0;
    while (!FullyExited && slept < 1000)
    {
        Sleep(50);
        slept += 50;
    }
    if (!FullyExited)
    {
        MessageBox(NULL, "Sorry, You can't configure while Running.", szTitleStuff, MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
        return 0;        
    }
    return 1;
}

/* Code from Vis_MegaSDK v1.03 */

// getmodule routine from the main header. Returns NULL if an invalid module was requested,
// otherwise returns either mod1, mod2 or mod3 depending on 'which'.
winampVisModule *getModule(int which)
{
    switch (which)
    {
        case 0  : return &mod1;
        case 1  : return &mod2;
        case 2  : return &mod3;
        case 3  : return &mod4;
        case 4  : return &mod5;
        default : return NULL ;  // and more .
    }
}

// configuration. Passed this_mod, as a "this" parameter. Allows you to make one configuration
// function that shares code for all your modules (you don't HAVE to use it though, you can make
// config1(), config2(), etc...)
void config(struct winampVisModule *this_mod)
{
    if (!WaitUntilPluginFinished())return;
    FullyExited = 0;
    MessageBox(this_mod->hwndParent,szAbout,szTitleStuff,MB_OK);
    FullyExited = 1;
}

void nofeat()
{
    MessageBox(NULL,szNotAvail,szTitleStuff,MB_OK);
}

// scrollable stuff
void makesong(const char *szSongTitleOri, char *szSongTitle)
{
    int i=0;
    szSongTitle[i]=0;
    if(!stats)return;
    for(;szSongTitleOri[i]&&i<=SPRLEN;i++)
    {
        szSongTitle[i]=szSongTitleOri[i];
    }
    szSongTitle[i]=0;
}

// fix extra data
void mktotstr()
{
    *szTotStr=0;
    if(!stats)return;
    if(*szSongTitle)
    {
        if(*szSongPos)
        {
            if(*szSongLen)
            {
                sprintf(szTotStr,"%s [%s/%s]",szSongTitle,szSongPos,szSongLen);
            }
            else
            {
                sprintf(szTotStr,"%s [%s]",szSongTitle,szSongPos);
            }
        }
        else
        {
            sprintf(szTotStr,"%s",szSongTitle);
        }
    }
    else
    {
        strcpy(szTotStr,"");
    }
}


// get extra unformatted data
void gettehdatau(HWND iamtoodamnconfused)
{
    if(!stats)return;
    GetWinampSongTitle(iamtoodamnconfused,szSongTitleOri,SZLEN);
    GetWinampSongPosAsText(iamtoodamnconfused,szSongPos);
    GetWinampSongLenAsText(iamtoodamnconfused,szSongLen);
    makesong(szSongTitleOri,szSongTitle);
    songpos=songlen=0.0f; //There !
}

// get extra formatted data
void gettehdata(HWND iamtoodamnconfused)
{
    gettehdatau(iamtoodamnconfused);
    mktotstr();
}

// double buffer renderer
void do_draw(HDC hdc)
{
    if(alerr||!alinited||!usedbuf) return;
    if(!hdc) return;
    // set palette
    set_palette_to_hdc(hdc, _current_palette);
    // copy doublebuffer to window
    draw_to_hdc(hdc, usedbuf, 0, 0);

}

// Allegro cleanup
void alquit()
{
    if(alinited)
    {
    if(!alerr)
    {
        if(usedbuf)
        destroy_bitmap(usedbuf);    // delete our doublebuffer 
        unselect_palette();
        allegro_exit();             // stop Allegro 
        alinited=0;
    }
    else
    {
        alerr=0;
    }
    alinited=0;
    }
}

// Allegro initialization.
int alinit(int width, int height,int bpp)
{

    // quit .
    if(alinited)
    {
        alquit();
        alinited=0; 
    }

    alerr=0;

    #ifdef ALLEGRO_STATICLINK
    // initialize Minimalized Allegro .
    if(install_allegro(SYSTEM_NONE,&errno,NULL))
    {
        alerr=1;
        return 1;
    }
    #else
    // initialize Allegro .
    if(install_allegro(SYSTEM_AUTODETECT,&errno,NULL))
    {
        alerr=1;
        return 1;
    }  
    #endif

    alinited=1;
    
    if(bpp>0)
    {
        set_color_depth(bpp);   
    }

    set_gdi_color_format();

    if(bpp<=0)
    {
        select_palette(default_palette);
    }

    // initialize Allegro's window .
    set_window_title(szTitleStuff);

    // create our doublebuffer 
    usedbuf=create_bitmap(width,height);

    // clear our doublebuffer 
    if(usedbuf)
    {
        clear_bitmap(usedbuf);
    }
    else 
    {
        alquit();
        return 1;
    }

    return 0;
}

// Processing cleanup
void btquit()
{
    if(btinited)
    {
    // cleanup misc stuff
    td_soundinfo[0].m_has_fallen=td_soundinfo[0].m_thresh=td_soundinfo[0].m_limit=td_soundinfo[0].timer==1;
    td_soundinfo[0].m_is_beat=td_soundinfo[0].m_last_hit=0;
    td_soundinfo[0].sum[0]=td_soundinfo[0].sum[1]=td_soundinfo[0].sum[2]=0;
    td_soundinfo[1].m_has_fallen=td_soundinfo[1].m_thresh=td_soundinfo[1].m_limit=td_soundinfo[1].timer=1;
    td_soundinfo[1].m_is_beat=td_soundinfo[1].m_last_hit=0;
    td_soundinfo[1].sum[0]=td_soundinfo[1].sum[1]=td_soundinfo[1].sum[2]=0;
    }
    // cleanup FFT
    FFT_CleanUp();
    btinited=0;
}

// Processing initialization
int btinit()
{
    // quit .
    if(btinited)
    {
        btquit();
        btinited=0;
    }

    // initialize FFT
    if(FFT_Init(576))
    {
        return -1;
    }

    // initialize misc stuff
    td_soundinfo[0].m_has_fallen=td_soundinfo[0].m_thresh=td_soundinfo[0].m_limit=td_soundinfo[0].timer==1;
    td_soundinfo[0].m_is_beat=td_soundinfo[0].m_last_hit=0;
    td_soundinfo[0].sum[0]=td_soundinfo[0].sum[1]=td_soundinfo[0].sum[2]=0;
    td_soundinfo[1].m_has_fallen=td_soundinfo[1].m_thresh=td_soundinfo[1].m_limit=td_soundinfo[1].timer=1;
    td_soundinfo[1].m_is_beat=td_soundinfo[1].m_last_hit=0;
    td_soundinfo[1].sum[0]=td_soundinfo[1].sum[1]=td_soundinfo[1].sum[2]=0;

    btinited=1;
    return 0;
}


// cleanup (opposite of init()). Destroys the window, unregisters the window class
void quit(struct winampVisModule *this_mod)
{
    if(config_x<0||config_x>MAXXVAL) config_x=0;
    if(config_y<0||config_y>MAXYVAL) config_y=0;
    if(stats<0||stats>1){stats=1;}
    config_write(this_mod);     // write configuration
    if(wa5flag) SendMessage(this_mod->hwndParent, WM_WA_IPC, 0, IPC_SETVISWND);
    // cleanup timer like counter
    fraps[0]=fraps[1]=0;
    // cleanup BMP stuff
    if(!alerr&&alinited&&boombox) destroy_bitmap(boombox);
    btquit();
    alquit();
    if (wa5flag&&w.me) 
    {   
    if(this_mod&&this_mod->hwndParent) SetForegroundWindow(this_mod->hwndParent);
    if(w.me) DestroyWindow(w.me);
    }
    else
    {
    if(hMainWnd) DestroyWindow(hMainWnd);    // delete our window
    }
    UnregisterClass(szAppName,this_mod->hDllInstance); // unregister window class
    FullyExited = 1;
    wa5flag=0;
}

// initialization. Registers our window class, creates our window, etc. Again, this
// one works for all modules, but you could make init1() and init2()...
// returns 0 on success, 1 on failure.
int init(struct winampVisModule *this_mod)
{
    int width;
    int height;
    int bpp;
    int style=WS_VISIBLE|WS_CHILDWINDOW|
        WS_OVERLAPPED|WS_CLIPCHILDREN|
        WS_CLIPSIBLINGS,
        wndstyle;
    HWND (*e)(embedWindowState *v);
    HWND parent=NULL;
    char *description;

    if (!WaitUntilPluginFinished())
    {
        return 1;        
    }

    // cleanup BMP stuff
    if(!alerr&&alinited&&boombox) destroy_bitmap(boombox);

    // quit .
    btquit();
    alquit();
    
    // unbuggy pre initializer .

    if(this_mod==&mod1)
    {
        SPRLEN=18;HEIGHT=320;WIDTH=288;BPP=0;
    }
        else if(this_mod==&mod2)
    {
        SPRLEN=18;HEIGHT=256;WIDTH=288;BPP=0;
    }
    else if(this_mod==&mod3)
    {
        SPRLEN=24;HEIGHT=50;WIDTH=320;BPP=0;srand(time(NULL));
    }
    else if(this_mod==&mod4)
    {
        SPRLEN=12;HEIGHT=240;WIDTH=240;BPP=16;
    }
    else if(this_mod==&mod5)
    {
        SPRLEN=20;HEIGHT=250;WIDTH=280;BPP=16;
    }
    else if(this_mod==NULL)
    {
        SPRLEN=0;HEIGHT=0;WIDTH=0;BPP=0;
    }

    width = WIDTH;
    height = HEIGHT;
    bpp=BPP;

    config_read(this_mod);

    if(config_x<0||config_x>MAXXVAL) config_x=0;
    if(config_y<0||config_y>MAXYVAL) config_y=0;
    if(stats<0||stats>1){stats=1;}

    // initialize timer like counter
    fraps[0]=fraps[1]=0;

    FullyExited = 0;
    {   
        WNDCLASS wc;
        wa5flag=1;

        if (wa5flag&&w.me) 
        {   
          if(this_mod&&this_mod->hwndParent) SetForegroundWindow(this_mod->hwndParent);
          if(w.me) DestroyWindow(w.me);
        }
        else
        {
        if(hMainWnd) DestroyWindow(hMainWnd);    // delete our window
        }

        // unregister window class if exists
        UnregisterClass(szAppName,this_mod->hDllInstance);
        
        w.r.left = config_x;
        w.r.top = config_y;
        w.r.right = config_x + width;
        w.r.bottom = config_y + height;

        *(void**)&e = (void *)SendMessage(this_mod->hwndParent,WM_WA_IPC,(LPARAM)0,IPC_GET_EMBEDIF);
        if (!e) //for WA5.
        {
        wa5flag=0;
        parent = this_mod->hwndParent;
        style=WS_VISIBLE|WS_SYSMENU;
        }
        else
        {
            parent = e(&w);
        }
        
        // set our window title
        if(wa5flag)
        {
        SetWindowText(w.me, this_mod->description); // set our window title
        }
        
        // Register our window class
        memset(&wc,0,sizeof(wc));
        wc.lpfnWndProc = WndProc;               // our window procedure
        wc.hInstance = this_mod->hDllInstance;  // hInstance of DLL
        wc.lpszClassName = szAppName;           // our window class name
    
        if (!RegisterClass(&wc)) 
        {
            MessageBox(this_mod->hwndParent,"Error registering Window Class",szTitleStuff,MB_OK);
            FullyExited = 1;
            return 1;
        }
    }

    if(wa5flag) //Window Style
    wndstyle=0;
    else 
    wndstyle=WS_EX_TOOLWINDOW|WS_EX_APPWINDOW;

    if(wa5flag) //Window Style
    description=NULL;
    else 
    description=this_mod->description;

    hMainWnd = CreateWindowEx(
        wndstyle,                           // exstyle.
        szAppName,                          // our window class name
        this_mod->description,              // use description for a window title
        style,                              // new thingy
        config_x,config_y,                  // screen position (read from config)
        width,height,                       // width & height of window (need to adjust client area later)
        parent,                             // parent window
        NULL,                               // no menu
        this_mod->hDllInstance,             // hInstance of DLL
        0); // no window creation data

    if (!hMainWnd) 
    {
        MessageBox(this_mod->hwndParent,"Error creating Window",szTitleStuff,MB_OK);
        FullyExited = 1;
        return 1;
    }

    SetWindowLong(hMainWnd,GWL_USERDATA,(LONG)this_mod); // set our user data to a "this" pointer
    
    if(!wa5flag)
    {   // adjust size of window to make the client area exactly with x height
        RECT r;
        int wp,hp;
        GetClientRect(hMainWnd,&r);
        wp=(width*2)-r.right;if(wp<0){wp=r.left;}if(wp<0){wp=0;}
        hp=(height*2)-r.bottom;if(hp<0){hp=r.top;}if(hp<0){hp=0;}
        SetWindowPos(hMainWnd,0,0,0,wp,hp,SWP_NOMOVE|SWP_NOZORDER);
    }

    // initialize
    if(alinit(width,height,bpp))
    {
        MessageBox(this_mod->hwndParent,"Error Initializing Allegro",szTitleStuff,MB_OK);
        if (wa5flag&&w.me) 
        {   
        DestroyWindow(w.me);
        }
        else
        {
        if(hMainWnd) DestroyWindow(hMainWnd);    // delete our window
        }
        UnregisterClass(szAppName,this_mod->hDllInstance);
        FullyExited = 1;
        return 1;
    }

    // initialize BMP stuff
    if(this_mod==&mod5)
    {
    char *p;
    GetModuleFileName(this_mod->hDllInstance,boom_file,MAX_PATH);
    p=boom_file + strlen(boom_file);
    while (p >= boom_file && *p != '\\') p--;
    if (++p >= boom_file) *p = 0;
    strcat(boom_file , szBBPFName);
    boombox = load_bitmap(boom_file,NULL);
    }
    
    // show the window
    if(wa5flag)
    SendMessage(this_mod->hwndParent, WM_WA_IPC, (int)hMainWnd, IPC_SETVISWND);

    ShowWindow(parent, SW_SHOWNORMAL);
    return 0;
}

/* Start Editing From here */

// render function for WaveForm Analyser. Returns 0 if successful, 1 if visualization should end.
int render1(struct winampVisModule *this_mod)
{
    int x;

    if(stats<0||stats>1){stats=1;}

    if(alerr||!alinited) return 1;
    if(stats)gettehdata(this_mod->hwndParent);

    if(usedbuf)clear_bitmap(usedbuf);
    // draw WaveForm
    for (x = 0; x < 288; x ++)
    {
        if(this_mod->nCh>1)
        {
            if(x)
            {
            if(usedbuf)
            line(usedbuf,
            x, (HEIGHT/2) + (this_mod->waveformData[1][x]^128) ,
            x-1, (HEIGHT/2) + (this_mod->waveformData[1][x-1]^128) ,
            makecol( 255, 0, 0 ));
            }
            else
            {
            if(usedbuf)
            putpixel(usedbuf,
            x, (HEIGHT/2) + (this_mod->waveformData[1][x]^128) ,
            makecol( 255, 0, 0 ));
            }
        }
            if(x)
            {
            if(usedbuf)
            line(usedbuf,
            x, (HEIGHT/2) - 1 - (this_mod->waveformData[0][x]^128) ,
            x-1, (HEIGHT/2) - 1 - (this_mod->waveformData[0][x-1]^128) ,
            makecol( 0, 0, 255 ));
            }
            else
            {
            if(usedbuf)
            putpixel(usedbuf,
            x, (HEIGHT/2) - 1 - (this_mod->waveformData[0][x]^128) ,
            makecol( 0, 0, 255 ));
            }
    }
    if(this_mod->nCh==1)
    {
        if(usedbuf)
        if(stats)
        textout_centre_ex(usedbuf, font, "Mono", WIDTH/2, HEIGHT/2, makecol(0,255,0),-1);
    }
    if(*szTotStr)
        if(usedbuf)
        if(stats)
        textout_ex(usedbuf, font, szTotStr, 0, 0, makecol(0,255,0),-1);
    if(hMainWnd)
    { 
        HDC hdc = GetDC(hMainWnd);
        do_draw(hdc);
        ReleaseDC(hMainWnd,hdc);
        //if(this_mod->nCh>1){fraps[1]++;}
        //fraps[0]++;
    }
    return 0;
}

// render function for Spectrum Analyser. Returns 0 if successful, 1 if visualization should end.
int render2(struct winampVisModule *this_mod)
{

    int x;

    if(stats<0||stats>1){stats=1;}
    
    if(alerr||!alinited) return 1;
    
    if(stats)gettehdata(this_mod->hwndParent);

    if(usedbuf)clear_bitmap(usedbuf);
    // draw Spectrum
    for (x = 0; x < 288; x ++)
    {
        if(this_mod->nCh>1)
        {
            if(usedbuf)
            line(usedbuf,
            x, (HEIGHT/2),
            x, (HEIGHT/2) + (this_mod->spectrumData[1][x]) ,
            makecol( 255, 255, 0));
        }
            if(usedbuf)
            line(usedbuf,
            x, (HEIGHT/2) - 1 ,
            x, (HEIGHT/2) - 1 - (this_mod->spectrumData[0][x]) ,
            makecol( 0, 255, 255));
    }
    if(this_mod->nCh==1)
    {
        if(usedbuf)
        if(stats)
        textout_centre_ex(usedbuf, font, "Mono", WIDTH/2, HEIGHT/2, makecol(255,0,255),-1);
    }
    if(*szTotStr)
        if(usedbuf)
        if(stats)
        textout_ex(usedbuf, font, szTotStr, 0, 0, makecol(255,0,255),-1);
    if(hMainWnd)
    { 
        HDC hdc = GetDC(hMainWnd);
        do_draw(hdc);
        ReleaseDC(hMainWnd,hdc);
        //if(this_mod->nCh>1){fraps[1]++;}
        //fraps[0]++;
    }
    return 0;
}

// render function for VU Analyser. Returns 0 if successful, 1 if visualization should end.
int render3(struct winampVisModule *this_mod)
{

    int x,txcolorg=0;
    int utotal1,total1,utotal2,total2,last,featteh; 

    if(stats<0||stats>1){stats=1;}
    
    if(alerr||!alinited) return 1;  
    
    if(stats)gettehdata(this_mod->hwndParent);
    if(stats)txcolorg=(rand()%256);
    if(usedbuf)clear_bitmap(usedbuf);

    // draw VU
    utotal2=total2=0; 
    if(this_mod->nCh>1)
    {
    last=this_mod->waveformData[1][0];
    for (x = 1; x < 288; x ++)
    {
    featteh=last-this_mod->waveformData[1][x];
    if(featteh<0){total2-=featteh;}
    else{total2+=featteh;}
    }
    total2 /= 288;
    if(total2>((WIDTH/2)-1))utotal2=0;
    else utotal2=(WIDTH/2)-1-total2;
    if(usedbuf)
        rectfill(usedbuf,WIDTH/2,0,(WIDTH/2)+utotal2,HEIGHT,makecol(255,255,255));

    total2=(total2*HEIGHT)/WIDTH;
    if(total2>((HEIGHT/2)-1))utotal2=0;
    else utotal2=(HEIGHT/2)-1-total2;
    }

    utotal1=total1=0;
    last=this_mod->waveformData[0][0];
    for (x = 1; x < 288; x ++)
    {
    featteh=last-this_mod->waveformData[0][x];
    if(featteh<0){total1-=featteh;}
    else{total1+=featteh;}
    }
    total1 /= 288;
    if(total1>((WIDTH/2)-1))utotal1=0;
    else utotal1=(WIDTH/2)-1-total1;
    if(usedbuf)
        rectfill(usedbuf,(WIDTH/2)-1,0,(WIDTH/2)-utotal1-1,HEIGHT,makecol(191,191,191));

    total1=(total1*HEIGHT)/WIDTH;
    if(total1>((HEIGHT/2)-1))utotal1=0;
    else utotal1=(HEIGHT/2)-1-total1;
    
    if(this_mod->nCh>1)
    {
        line(usedbuf,0,(HEIGHT/2)+utotal2,WIDTH,(HEIGHT/2)+utotal2,makecol(191,191,191));
    }

    line(usedbuf,0,(HEIGHT/2)-utotal1-1,WIDTH,(HEIGHT/2)-utotal1-1,makecol(255,255,255));

    if(this_mod->nCh==1)
    {
        if(usedbuf)
        if(stats)
        textout_centre_ex(usedbuf, font, "Mono", WIDTH/2, HEIGHT/2, makecol(txcolorg,txcolorg,txcolorg),-1);
    }
    if(*szTotStr)
        if(usedbuf)
        if(stats)
        textout_ex(usedbuf, font, szTotStr, 0, 0, makecol(txcolorg,txcolorg,txcolorg),-1);
    if(hMainWnd)
    { 
        HDC hdc = GetDC(hMainWnd);
        do_draw(hdc);
        ReleaseDC(hMainWnd,hdc);
        //if(this_mod->nCh>1){fraps[1]++;}
        //fraps[0]++;
    }
    return 0;
}

// render function for The Grind . Returns 0 if successful, 1 if visualization should end.
int render4(struct winampVisModule *this_mod)
{

    int x;
    int COL,GV; 

    if(stats<0||stats>1){stats=1;}
    
    if(alerr||!alinited) return 1;
    
    if(stats)gettehdata(this_mod->hwndParent);
    if(usedbuf)clear_bitmap(usedbuf);

    //Should be useful .
    if(!btinited)
    {
        if(btinit())
        {
            return 1;
        }
        fraps[0]=fraps[1]=0;
    }

    //Beat Based Change .
    analyze(this_mod);
    
    if(this_mod->nCh>1&&td_soundinfo[1].m_is_beat)fraps[1]++;
        
    if(td_soundinfo[0].m_is_beat)fraps[0]++;

    // draw Grind
    for (x = 287; x >=0; x --)
    {
    if(this_mod->nCh>1)
    {
    GV=(int)(0.22*(unsigned char)(this_mod->waveformData[1][x]&255));
    COL = makecol(GV,GV,GV);
    if(usedbuf)
    {
        if(fraps[1]%2) ellipsefill(usedbuf,(int)(3*WIDTH/4),(int)(3*HEIGHT/4),(int)((x*WIDTH)/(4*288)),(int)((x*HEIGHT)/(4*288)),COL);
        else ellipsefill(usedbuf,(int)(3*WIDTH/4),(int)(HEIGHT/4),(int)((x*WIDTH)/(4*288)),(int)((x*HEIGHT)/(4*288)),COL);
    }
    }
    GV=(int)(0.22*(unsigned char)(this_mod->waveformData[0][x]&255));
    COL = makecol(GV,GV,GV);
    if(usedbuf)
    {
        if(fraps[0]%2) ellipsefill(usedbuf,(int)(WIDTH/4),(int)(HEIGHT/4),(int)((x*WIDTH)/(4*288)),(int)((x*HEIGHT)/(4*288)),COL);
        else ellipsefill(usedbuf,(int)(WIDTH/4),(int)(3*HEIGHT/4),(int)((x*WIDTH)/(4*288)),(int)((x*HEIGHT)/(4*288)),COL);
    }
    }
    if(this_mod->nCh==1)
    {
        if(usedbuf)
        if(stats)
        textout_centre_ex(usedbuf, font, "Mono", WIDTH/2, HEIGHT/2, makecol(255,191,127),-1);
    }
    if(*szTotStr)
        if(usedbuf)
        if(stats)
        textout_ex(usedbuf, font, szTotStr, 0, 0, makecol(255,191,127),-1);
    if(hMainWnd)
    { 
        HDC hdc = GetDC(hMainWnd);
        do_draw(hdc);
        ReleaseDC(hMainWnd,hdc);
    }
    return 0;
}

// render function for Boom Box - Explosion. Returns 0 if successful, 1 if visualization should end.
int render5(struct winampVisModule *this_mod)
{
    char szSongTitleFunk[SZLEN];
    int x, tx, OFFSET=10, THRESHHOLD=128;
    float theta, step;

    if(stats<0||stats>1){stats=1;}
    if(boom_what<0||boom_what>1){boom_what=1;}
    if(boom_flag<0||boom_flag>1){boom_flag=1;}
    
    if(alerr||!alinited) return 1;
    
    if(stats)gettehdatau(this_mod->hwndParent);
    if(usedbuf)clear_bitmap(usedbuf);

    //Should be useful .
    if(!btinited)
    {
        if(btinit())
        {
            return 1;
        }
    }
    
    //Beat Based Change .Let it slow down if reqd   
    analyze(this_mod);
    if(this_mod->nCh>1&&td_soundinfo[1].m_is_beat)fraps[1]++;
    if(td_soundinfo[0].m_is_beat)fraps[0]++;
    
    clear_bitmap(usedbuf);

    if(boom_flag)
    {
     if(boombox)
         if(usedbuf)
            blit(boombox,usedbuf,0,0,(usedbuf->w - boombox->w)/2,(usedbuf->h - boombox->h)/2,usedbuf->w,usedbuf->h);
    }

    if(boom_what)
    {
     if(boom_flag)
     {
         if(usedbuf)
            textout_ex(usedbuf,font,"Waveform",200,10,makecol(200,200,0),-1);
     }
    }
    else
    {
     if(boom_flag)
     {
         if(usedbuf)
            textout_ex(usedbuf,font,"Spectrum",200,10,makecol(200,200,0),-1);
     }
    }

    // get boom parameters
    if(boom_what)
    {
        if(this_mod->nCh>1)boom_data[1] = (this_mod->waveformData[1]);
        boom_data[0] = (this_mod->waveformData[0]);
    }
    else
    {
        if(this_mod->nCh>1) boom_data[1] = (this_mod->spectrumData[1]);
        boom_data[0] = (this_mod->spectrumData[0]);
    }

    // draw the speaker explosions
    for (x = 0; x < 288; x ++)
    {

    theta = (x*(2*PI))/288;

        if(boom_what)
        {
        if(x>0)tx=x-1;
        else tx=287; 
        if(usedbuf)
            for(step=1;step<5;step++)
            {
               line(usedbuf,
               (WIDTH/4 + OFFSET) + (int)(((step*0.5)/5)*(boom_data[0][x]^128) * cos(theta)),
               (HEIGHT/3) + (int)(((step*0.5)/5)*(boom_data[0][x]^128) * sin(theta)),
               (WIDTH/4 + OFFSET) + (int)(((step*0.5)/5)*(boom_data[0][tx]^128) * cos(theta-(2*PI/288))),
               (HEIGHT/3) + (int)(((step*0.5)/5)*(boom_data[0][tx]^128) * sin(theta-(2*PI/288))),
                makecol(200+(step*8),100+(step*2),50+step));
            }
        }
        else
        {
        if(usedbuf)
               circle(usedbuf,(WIDTH/4)+OFFSET,HEIGHT/3,(int)(0.3*(boom_data[0][x]^64)),makecol(50+((x*200)/288),50+((x*75)/288),0));
        }

    if(this_mod->nCh>1)
    {
            if(boom_what)
            {
             if(x>0)tx=x-1;
             else tx=287; 
             if(usedbuf)
                for(step=1;step<5;step++)
                {
                line(usedbuf,
                ((3*WIDTH/4) - OFFSET) - (int)(((step*0.5)/5)*(boom_data[1][x]^128) * cos(theta)),
                (HEIGHT/3) + (int)(((step*0.5)/5)*(boom_data[1][x]^128) * sin(theta)),
                ((3*WIDTH/4) - OFFSET) - (int)(((step*0.5)/5)*(boom_data[1][tx]^128) * cos(theta-(2*PI/288))),
                (HEIGHT/3) + (int)(((step*0.5)/5)*(boom_data[1][tx]^128) * sin(theta-(2*PI/288))),
                makecol(200+(step*8),100+(step*2),50+step));
                }
            }
            else
            {
            if(usedbuf)
               circle(usedbuf,(3*WIDTH/4)-OFFSET,HEIGHT/3,(int)(0.3*(boom_data[1][x]^64)),makecol(50+((x*200)/288),50+((x*75)/288),0));
            }
        }
    }

    if(boom_flag)
    {
     if(this_mod->nCh>1)
     {
      if(usedbuf&&boombox)
      {
      if(fraps[1]%2)
      {
           ellipsefill(usedbuf,36,233,5,7,makecol(0,255,0));
      }
      else
      {
           ellipsefill(usedbuf,36,233,5,7,makecol(0,127,0));
      }
      }
     }
     if(usedbuf&&boombox)
     {
     if(fraps[0]%2)
     {
           ellipsefill(usedbuf,34,210,5,7,makecol(255,0,0));
     }
     else
     {
           ellipsefill(usedbuf,34,210,5,7,makecol(127,0,0));
     }
     }
    }

    if(this_mod->nCh==1)
    {
      if(stats)
      {
        if(usedbuf)
            textout_ex(usedbuf, font, "MONO", 205, 225, makecol(255,63,0),-1);
      }
    }

    if(stats)
    {
    if(*szSongTitle)
    {
    if(usedbuf)
    {    
        makescroll(szSongTitleOri,szSongTitleFunk,(fraps[0]+fraps[1]),SPRLEN); // Scroll according to Beat...
        textout_ex(usedbuf, font, szSongTitleFunk, 75, 210, makecol(0,191,63),-1);       
    }
    }

    if(*szSongPos)
    {
        if(*szSongLen)
        {
            sprintf(szTotStr,"[%s/%s]",szSongPos,szSongLen);
        }
        else
        {
            sprintf(szTotStr,"[%s]",szSongPos);
        }
    }
    else
    {
        *szTotStr=0;
    }

    if(*szTotStr)
    {
    if(usedbuf)
        textout_ex(usedbuf, font, szTotStr, 92, 225, makecol(0,191,63),-1);
    }
    }

    if(hMainWnd)
    { 
        HDC hdc = GetDC(hMainWnd);
        do_draw(hdc);
        ReleaseDC(hMainWnd,hdc);
    }

    return 0;
}

// window procedure for our window
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:     return 0;
        case WM_ERASEBKGND: return 0;
        case WM_PAINT:
            {   // update from doublebuffer
                PAINTSTRUCT ps;
                if(hwnd)
                {
                    HDC hdc = BeginPaint(hwnd,&ps);
                    do_draw(hdc);
                    if(wa5flag)
                    {
                    RECT r;
                    GetClientRect(hwnd,&r);
                    {
                    //Clear Stuff.
                    RECT x={r.left+WIDTH, r.top, r.right, r.bottom};
                    RECT y={r.left, r.top+HEIGHT, r.right, r.bottom};
                    FillRect(hdc, &x, GetStockObject(OBRUSH));
                    FillRect(hdc, &y, GetStockObject(OBRUSH));
                    }
                    }
                    EndPaint(hwnd,&ps);
                }
            }
        return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
        case WM_CHAR:
            {
                switch (wParam)
                {
                // OSD Toggle:
                case 'y':
                case 'Y':
                    stats=1-stats;
                    break;
                // Boom Box Mode Toggle:
                case 'w':
                case 'W':
                    boom_what = 1 - boom_what;
                    break;
                 // Boom Box BG Toggle:
                 case 't':
                 case 'T':
                    boom_flag = 1 - boom_flag;
                    break;
                }
            }
        case WM_KEYDOWN: // pass keyboard messages to main winamp window (for processing)
        case WM_KEYUP:
            {   // get this_mod from our window's user data
                winampVisModule *this_mod = (winampVisModule *) GetWindowLong(hwnd,GWL_USERDATA);
                PostMessage(this_mod->hwndParent,message,wParam,lParam);
            }
        return 0;
        case WM_MOVE:
            {   // get config_x and config_y for configuration
                RECT r;
                case WM_WINDOWPOSCHANGING:
                GetWindowRect(hMainWnd,&r);

                if(r.left<0)
                {
                    if((config_x=(r.right-(2*WIDTH)))<0)
                    config_x=0;
                }
                else {config_x=r.left;}

                if(r.top<0)
                {
                    if((config_y=(r.bottom-(2*HEIGHT)))<0)
                    config_y=0;
                }
                else {config_y=r.top;}

                if(config_x>MAXXVAL){config_x=0;}
                if(config_y>MAXXVAL){config_y=0;}

                if(stats<0||stats>1){stats=1;}
            }
            return 0;
            case WM_COMMAND: //Unused.
            {
             int id = LOWORD(wParam);
             if(wa5flag)
             {
             switch(id)
             {
             case ID_VIS_NEXT: nofeat(); break;
             case ID_VIS_PREV: nofeat(); break;
             case ID_VIS_FS:  nofeat(); break;
             case ID_VIS_CFG: nofeat(); break;
             case ID_VIS_MENU: nofeat(); break;
             }
             }
            }
     }
    return DefWindowProc(hwnd,message,wParam,lParam);
}

/* Code from Vis_MegaSDK v1.03 */

// Rate Adjuster .
float AdjustRateTo(float per_frame_decay_rate_at_num1, float num1, float actual_num)
{
    // returns the equivalent per-frame decay rate at actual_num
    
    // basically, do all your testing at num1 and get a good decay rate;
    // then, in the real application, adjust that rate by the actual num  each time you use it.
    
    float per_second_decay_rate_at_num1 = pow(per_frame_decay_rate_at_num1, num1);
    float per_frame_decay_rate_at_num2 = pow(per_second_decay_rate_at_num1, 1.0f/actual_num);

    return per_frame_decay_rate_at_num2;
}

// Accurate Sound Analyser .
void analyze(struct winampVisModule *this_mod)
{
    int i=0,old_i=-1;

    float med_mix[2]  = {0.965f,0.965f}; //Originally 0.91
    float long_mix[2] = {0.985f,0.985f}; //Originally 0.96
    float avg_mix[2],vol[2],decay;

    td_soundinfo[1].m_is_beat = 0;
    td_soundinfo[0].m_is_beat = 0;

    if(this_mod->waveformNch<1||this_mod->nCh<1)
    {
        return;
    }

    if(!btinited)
    {
        if(btinit()) return;
    }

    for (; i<576; i++)
    {
        if(this_mod->nCh>1)
        {td_soundinfo[1].fWaveform[i] = (float)((this_mod->waveformData[1][i] ^ 128) - 128);}
        td_soundinfo[0].fWaveform[i] = (float)((this_mod->waveformData[0][i] ^ 128) - 128);
        // damp the input into the FFT a bit, to reduce high-frequency noise:
        if(this_mod->nCh>1)
        {if(old_i>=0)temp_wave[1][i] = 0.5f*(td_soundinfo[1].fWaveform[i] + td_soundinfo[1].fWaveform[old_i]);}
        if(old_i>=0)temp_wave[0][i] = 0.5f*(td_soundinfo[0].fWaveform[i] + td_soundinfo[0].fWaveform[old_i]);
        old_i = i;
    }
    if(this_mod->nCh>1)
    {time_to_frequency_domain(temp_wave[1], td_soundinfo[1].fSpectrum);}
    time_to_frequency_domain(temp_wave[0], td_soundinfo[0].fSpectrum);

    // sum channels spectrum up into 3 bands
    for (i=0; i<3; i++)
    {
        float exp = 2.1f;
        int j;

        int start = (int)(NFREQ*0.5f*pow(i/3.0f, exp));
        int ends  = (int)(NFREQ*0.5f*pow((i+1)/3.0f, exp));

        if(this_mod->nCh>1)
        {
        td_soundinfo[1].imm[i] = 0;
        for (j=start; j<ends; j++)
            td_soundinfo[1].imm[i] += td_soundinfo[1].fSpectrum[j];
        td_soundinfo[1].imm[i] /= (float)(ends-start);
        }

        td_soundinfo[0].imm[i] = 0;
        for (j=start; j<ends; j++)
            td_soundinfo[0].imm[i] += td_soundinfo[0].fSpectrum[j];
        td_soundinfo[0].imm[i] /= (float)(ends-start);
        
    }

    // some code to find empirical long-term averages for imm[0..2]:

    if(this_mod->nCh>1)
    {
    
    td_soundinfo[1].sum[0] = (bt_tm_dec*td_soundinfo[1].sum[0])+td_soundinfo[1].imm[0];
    td_soundinfo[1].sum[1] = (bt_tm_dec*td_soundinfo[1].sum[1])+td_soundinfo[1].imm[1];
    td_soundinfo[1].sum[2] = (bt_tm_dec*td_soundinfo[1].sum[2])+td_soundinfo[1].imm[2];

    if(td_soundinfo[1].timer>min_infi)
    {
    td_soundinfo[1].infi_avg[0] = td_soundinfo[1].sum[0]/(td_soundinfo[1].timer-min_infi);
    td_soundinfo[1].infi_avg[1] = td_soundinfo[1].sum[1]/(td_soundinfo[1].timer-min_infi);
    td_soundinfo[1].infi_avg[2] = td_soundinfo[1].sum[2]/(td_soundinfo[1].timer-min_infi);
    }
    else
    {
    td_soundinfo[1].infi_avg[0]=0.270f;   // 0.244 0.270f 0.276 
    td_soundinfo[1].infi_avg[1]=0.343f;   // 0.333 0.343f 0.347
    td_soundinfo[1].infi_avg[2]=0.295f;   // 0.323 0.295f 0.290
    }

    td_soundinfo[1].timer++;
    }    

    td_soundinfo[0].sum[0] = (bt_tm_dec*td_soundinfo[0].sum[0])+td_soundinfo[0].imm[0];
    td_soundinfo[0].sum[1] = (bt_tm_dec*td_soundinfo[0].sum[1])+td_soundinfo[0].imm[1];
    td_soundinfo[0].sum[2] = (bt_tm_dec*td_soundinfo[0].sum[2])+td_soundinfo[0].imm[2];

    if(td_soundinfo[0].timer>min_infi)
    {
    td_soundinfo[0].infi_avg[0] = td_soundinfo[0].sum[0]/(td_soundinfo[0].timer-min_infi);
    td_soundinfo[0].infi_avg[1] = td_soundinfo[0].sum[1]/(td_soundinfo[0].timer-min_infi);
    td_soundinfo[0].infi_avg[2] = td_soundinfo[0].sum[2]/(td_soundinfo[0].timer-min_infi);
    }
    else
    {
    td_soundinfo[0].infi_avg[0]=0.270f;   // 0.244 0.270f 0.276 
    td_soundinfo[0].infi_avg[1]=0.343f;   // 0.333 0.343f 0.347
    td_soundinfo[0].infi_avg[2]=0.295f;   // 0.323 0.295f 0.290
    }

    td_soundinfo[0].timer++;

    // multiply by long-term, empirically-determined
    // inverse averages: 500s:  1000s:

    if(this_mod->nCh>1)
    {
    td_soundinfo[1].imm[0] /= td_soundinfo[1].infi_avg[0];
    td_soundinfo[1].imm[1] /= td_soundinfo[1].infi_avg[1];
    td_soundinfo[1].imm[2] /= td_soundinfo[1].infi_avg[2];
    }

    td_soundinfo[0].imm[0] /= td_soundinfo[0].infi_avg[0];
    td_soundinfo[0].imm[1] /= td_soundinfo[0].infi_avg[1];
    td_soundinfo[0].imm[2] /= td_soundinfo[0].infi_avg[2];

    
    // temporal blending to create attenuated and super-attenuated versions

    if(this_mod->nCh>1)
    {
    med_mix[1]  = AdjustRateTo( med_mix[1], 14.0f, m_used);
    long_mix[1] = AdjustRateTo(long_mix[1], 14.0f, m_used);
    }
    
    med_mix[0]  = AdjustRateTo( med_mix[0], 14.0f, m_used);
    long_mix[0] = AdjustRateTo(long_mix[0], 14.0f, m_used);

    for (i=0; i<3; i++)
    {
            if(this_mod->nCh>1)
            {
            if (td_soundinfo[1].imm[i] > td_soundinfo[1].avg[i])
                avg_mix[1] = AdjustRateTo(0.2f,  14.0f, m_used);
            else
                avg_mix[1] = AdjustRateTo(0.5f,  14.0f, m_used);

            td_soundinfo[1].avg[i] = td_soundinfo[1].avg[i]*avg_mix[1] + td_soundinfo[1].imm[i]*(1-avg_mix[1]);
            td_soundinfo[1].med_avg[i]  =  td_soundinfo[1].med_avg[i]*(med_mix[1]) + td_soundinfo[1].imm[i]*(1-med_mix[1]);
            td_soundinfo[1].long_avg[i] = td_soundinfo[1].long_avg[i]*(long_mix[1]) + td_soundinfo[1].imm[i]*(1-long_mix[1]);
            }

            if (td_soundinfo[0].imm[i] > td_soundinfo[0].avg[i])
                avg_mix[0] = AdjustRateTo(0.2f,  14.0f, m_used);
            else
                avg_mix[0] = AdjustRateTo(0.5f,  14.0f, m_used);

            td_soundinfo[0].avg[i] = td_soundinfo[0].avg[i]*avg_mix[0] + td_soundinfo[0].imm[i]*(1-avg_mix[0]);
            td_soundinfo[0].med_avg[i]  =  td_soundinfo[0].med_avg[i]*(med_mix[0]) + td_soundinfo[0].imm[i]*(1-med_mix[0]);
            td_soundinfo[0].long_avg[i] = td_soundinfo[0].long_avg[i]*(long_mix[0]) + td_soundinfo[0].imm[i]*(1-long_mix[0]);
    }

    if(this_mod->nCh>1)
    {
    vol[1] = 0.3333f*(td_soundinfo[1].avg[0]/td_soundinfo[1].med_avg[0] + td_soundinfo[1].avg[1]/td_soundinfo[1].med_avg[1] + td_soundinfo[1].avg[2]/td_soundinfo[1].med_avg[2]);
    }

    vol[0] = 0.3333f*(td_soundinfo[0].avg[0]/td_soundinfo[0].med_avg[0] + td_soundinfo[0].avg[1]/td_soundinfo[0].med_avg[1] + td_soundinfo[0].avg[2]/td_soundinfo[0].med_avg[2]);
        
    decay = AdjustRateTo(0.997f, 20.0f, m_used);

    if(this_mod->nCh>1)
    {
    td_soundinfo[1].m_thresh = td_soundinfo[1].m_thresh*decay+      
    td_soundinfo[1].m_limit*(1-decay);
    }

    td_soundinfo[0].m_thresh = td_soundinfo[0].m_thresh*decay+      
    td_soundinfo[0].m_limit*(1-decay);

    if(this_mod->nCh>1)
    {
        if (!td_soundinfo[1].m_has_fallen)
        {
            // wait for it to get a bit quieter before allowing another beat
            if (vol[1] < td_soundinfo[1].m_thresh)
            { 
                td_soundinfo[1].m_has_fallen   = 1;
                td_soundinfo[1].m_thresh       = td_soundinfo[1].m_last_hit*1.03f;
                td_soundinfo[1].m_limit        = 1.1f;
            }
        }
        else
        {
            // signal fell; now wait for vol to reach m_thresh, then signal a beat.
            if (vol[1] > td_soundinfo[1].m_thresh && vol[1] > 0.2f)
            {
                td_soundinfo[1].m_is_beat      = 1;
                td_soundinfo[1].m_last_hit     = vol[1];
                td_soundinfo[1].m_has_fallen   = 0;
                td_soundinfo[1].m_thresh       = max(1.0f, vol[1]*0.85f);  
                td_soundinfo[1].m_limit        = max(1.0f, vol[1]*0.70f);
            }
        }
    }

        if (!td_soundinfo[0].m_has_fallen)
        {
            // wait for it to get a bit quieter before allowing another beat
            if (vol[0] < td_soundinfo[0].m_thresh)
            { 
                td_soundinfo[0].m_has_fallen   = 1;
                td_soundinfo[0].m_thresh       = td_soundinfo[0].m_last_hit*1.03f;
                td_soundinfo[0].m_limit        = 1.1f;
            }
        }
        else
        {
            // signal fell; now wait for vol to reach m_thresh, then signal a beat.
            if (vol[0] > td_soundinfo[0].m_thresh && vol[0] > 0.2f)
            {
                td_soundinfo[0].m_is_beat      = 1;
                td_soundinfo[0].m_last_hit     = vol[0];
                td_soundinfo[0].m_has_fallen   = 0;
                td_soundinfo[0].m_thresh       = max(1.0f, vol[0]*0.85f);  
                td_soundinfo[0].m_limit        = max(1.0f, vol[0]*0.70f);
            }
       }
}
