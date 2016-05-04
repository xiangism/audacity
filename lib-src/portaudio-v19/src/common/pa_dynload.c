/*
 * $Id: pa_dynlink.c 1339 2008-02-15 07:50:33Z rossb $
 * Portable Audio I/O Library
 * dynamic library helper
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 2008 Ross Bencina
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however, 
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also 
 * requested that these non-binding requests be included along with the 
 * license above.
 */

/** @file
 @ingroup common_src

 @brief dynamic library helper functions.
*/

#if defined(WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "pa_debugprint.h"
#include "pa_dynload.h"

#if !defined(NULL)
#define NULL 0
#endif

paDynamicLib PaDL_Load( char *name )
{
    paDynamicLib lib;

#if defined(WIN32)
    lib = LoadLibraryA(name);
#else
    lib = dlopen(name, RTLD_LAZY);
#endif

    if (!lib) {
#if defined(WIN32)
        PA_DEBUG(("Couldn't load %s, error code: %d\n", name, GetLastError()));
#else
        PA_DEBUG(("Couldn't load %s, error: %s\n", name, dlerror()));
#endif
    }
        
    return lib;
}

void PaDL_Unload( paDynamicLib lib )
{
#if defined(WIN32)
    FreeLibrary(lib);
#else
    dlclose(lib);
#endif
}

void *PaDL_FindSymbol( paDynamicLib lib, char *name )
{
    void *addr;

#if defined(WIN32)
    addr = (void *) GetProcAddress(lib, name);
#else
    addr = dlsym(lib, name);
#endif

    if (addr == NULL) {
#if defined(WIN32)
        PA_DEBUG(("Couldn't find %s function, error code: %d\n", name, GetLastError()));
#else
        PA_DEBUG(("Couldn't find %s function, error: %s\n", name, dlerror()));
#endif
    }
        
    return addr;
}
