#include <stdio.h>
#include <string.h>
#include "vis_mod.h"
#include "wa_ipc.h"

void getWinampSongTitle(HWND hWndWinamp, char *titleBuffer, int nSize) {
    char *p;
    int pos;

    titleBuffer[0] = 0;

    if (GetWindowText(hWndWinamp, titleBuffer, nSize)) {
        if (strlen(titleBuffer) > 9) {
            int checkPosition = strlen(titleBuffer) - 9;
            if (lstrcmp(" - Winamp", (char *)(titleBuffer + checkPosition)) == 0)
                titleBuffer[checkPosition] = 0;
        }

        if (strlen(titleBuffer) > 18) {
            int checkPosition = strlen(titleBuffer) - 18;
            if (lstrcmp(" - Winamp [Paused]", (char *)(titleBuffer + checkPosition)) == 0)
                titleBuffer[checkPosition] = 0;
        }

        if (strlen(titleBuffer) > 19) {
            int checkPosition = strlen(titleBuffer) - 19;
            if (lstrcmp(" - Winamp [Stopped]", (char *)(titleBuffer + checkPosition)) == 0)
                titleBuffer[checkPosition] = 0;
        }

        p = titleBuffer;
        while (*p >= '0' && *p <= '9') p++;
        if (*p == '.' && *(p+1) == ' ') {
            p += 2;
            pos = 0;
            while (*p != 0) {
                titleBuffer[pos++] = *p;
                p++;
            }
            titleBuffer[pos++] = 0;
        }

        {
            int pos = 0, shiftIndex;
            int len = strlen(titleBuffer);
            while (titleBuffer[pos]) {
                if (titleBuffer[pos] == '&') {
                    for (shiftIndex=len; shiftIndex>=pos; shiftIndex--)
                        titleBuffer[shiftIndex+1] = titleBuffer[shiftIndex];
                    len++;
                    pos++;
                }
                pos++;
            }
        }
    }
}

void getWinampSongPosAsText(HWND hWndWinamp, char *positionBuffer) {
    int songPosMs,minutes,seconds,deciSeconds;
    float timeSeconds;
    positionBuffer[0] = 0;
    songPosMs = SendMessage(hWndWinamp,WM_USER,0,105);
    if (songPosMs > 0) {
        timeSeconds = songPosMs*0.001f;
        minutes = (int)(timeSeconds/60);
        timeSeconds -= minutes*60;
        seconds = (int)timeSeconds;
        timeSeconds -= seconds;
        deciSeconds = (int)(timeSeconds*100);
        sprintf(positionBuffer, "%d:%02d.%02d", minutes, seconds, deciSeconds);
    }
}

void getWinampSongLenAsText(HWND hWndWinamp, char *lengthBuffer) {
    int songLengthMs;
    lengthBuffer[0] = 0;
    songLengthMs = SendMessage(hWndWinamp,WM_USER,1,105)*1000;
    if (songLengthMs > 0) {
        int lengthSeconds = songLengthMs/1000;
        int minutes = lengthSeconds/60;
        int seconds = lengthSeconds - minutes*60;
        sprintf(lengthBuffer, "%d:%02d", minutes, seconds);
    }
}

int waitUntilPluginFinished() {
    int sleptMs = 0;
    while (!fullyExited && sleptMs < 1000) {
        Sleep(50);
        sleptMs += 50;
    }
    if (!fullyExited) {
        MessageBox(NULL, "Sorry, You can't configure while Running.", titleText, MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
        return 0;
    }
    return 1;
}

void truncateSongTitle(const char *originalTitle, char *truncatedTitle) {
    int i=0;
    truncatedTitle[i]=0;
    if(!showStats)return;
    for(;originalTitle[i]&&i<=scrollLength;i++) {
        truncatedTitle[i]=originalTitle[i];
    }
    truncatedTitle[i]=0;
}

void makeDisplayString() {
    *displayString=0;
    if(!showStats)return;
    if(*currentSongTitle) {
        if(*positionText) {
            if(*lengthText) {
                snprintf(displayString,sizeof(displayString),"%s [%s/%s]",currentSongTitle,positionText,lengthText);
            }
            else
            {
                snprintf(displayString,sizeof(displayString),"%s [%s]",currentSongTitle,positionText);
            }
        }
        else
        {
            snprintf(displayString,sizeof(displayString),"%s",currentSongTitle);
        }
    }
    else
    {
        strcpy(displayString,"");
    }
}

void updateUnformattedSongData(HWND hWnd) {
    if(!showStats)return;
    getWinampSongTitle(hWnd,originalSongTitle,SZLEN);
    getWinampSongPosAsText(hWnd,positionText);
    getWinampSongLenAsText(hWnd,lengthText);
    truncateSongTitle(originalSongTitle,currentSongTitle);
    songPositionSeconds=songLengthSeconds=0.0f;
}

void updateSongData(HWND hWnd) {
    updateUnformattedSongData(hWnd);
    makeDisplayString();
}

void makeScrollText(char source[SZLEN],char destination[SZLEN],int scrollPosition,int windowSize) {
    char combinedText[SZLEN*3];
    int length=0,j=0,startIndex;
    for(;source[length];++length);
    *destination=0;
    strcpy(combinedText,source);
    strcat(combinedText,separatorFormat);
    strcat(combinedText,source);
    if(length<=0||windowSize<=0) return;
    startIndex=scrollPosition%length;
    for(;j<windowSize;++j)
        destination[j]=combinedText[startIndex+j];
    destination[j]=0;
}
