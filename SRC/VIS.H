/*
  LICENSE
  -------
  Copyright (C) 1999-2002 Nullsoft, Inc.

  This source code is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this source code or the software it produces.

  Permission is granted to anyone to use this source code for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this source code must not be misrepresented; you must not
     claim that you wrote the original source code.  If you use this source code
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original source code.
  3. This notice may not be removed or altered from any source distribution.
*/

// notes:
// any window that remains in foreground should optimally pass 
// keystrokes to the parent (winamp's) window, so that the user
// can still control it. unless escape is hit, or some option 
// key specific to the vis is hit. As for storing configuration,
// Configuration data should be stored in <dll directory>\plugin.ini
// Look at the example plugin for a framework.

// ints are 32 bits, and structure members are aligned on the default 8 byte boundaries
// tested with VC++ 4.2 and 5.0


typedef struct winampVisModule {
  char *description; // description of module
  HWND hwndParent;   // parent window (filled in by calling app)
  HINSTANCE hDllInstance; // instance handle to this DLL (filled in by calling app)
  int sRate;         // sample rate (filled in by calling app)
  int nCh;             // number of channels (filled in...)
  int latencyMs;     // latency from call of RenderFrame to actual drawing
                     // (calling app looks at this value when getting data)
  int delayMs;       // delay between calls in ms

  // the data is filled in according to the respective Nch entry
  int spectrumNch;
  int waveformNch;
  unsigned char spectrumData[2][576];
  unsigned char waveformData[2][576];

  void (*Config)(struct winampVisModule *this_mod);  // configuration dialog
  int (*Init)(struct winampVisModule *this_mod);     // 0 on success, creates window, etc
  int (*Render)(struct winampVisModule *this_mod);   // returns 0 if successful, 1 if vis should end
  void (*Quit)(struct winampVisModule *this_mod);    // call when done

  void *userData; // user data, optional
} winampVisModule;

typedef struct {
  int version;       // VID_HDRVER
  char *description; // description of library
  winampVisModule* (*getModule)(int);
} winampVisHeader;

// exported symbols
typedef winampVisHeader* (*winampVisGetHeaderType)();

// version of current module (0x101 == 1.01)
#define VIS_HDRVER 0x101

//BOOL CALLBACK ConfigDialogProc(HWND  hwndDlg, UINT  uMsg, WPARAM  wParam, LPARAM  lParam);
