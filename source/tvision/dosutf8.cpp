/*-------------------------------------------------------------------*/
/* filename -       dosutf8.cpp                                      */
/*                                                                   */
/* function(s)                                                       */
/*          initDosUtf8 - asks the DOS for UTF-8 file names          */
/*-------------------------------------------------------------------*/

// DOS only (Borland C++). Elsewhere this file is empty.
//
// A DOS that provides the AMIS (INT 2Dh) extension "DOS-UTF8/NAMES" can give
// and take the file names in UTF-8: every function of INT 21h that works on
// long names (AH=71h) then returns and accepts UTF-8 for this process. This is
// what DOSBox-X does since https://github.com/joncampbell123/dosbox-x/pull/6632
// (the option "utf8 file names"), and what the DOS of go2dos does.
//
// The program asks for it once, at the start (the first TApplication).
// Setting the environment variable TV_DOS_UTF8_NAMES=0 keeps the code page of
// the DOS. A DOS without the provider is not affected in any way.

#if defined( __BORLANDC__ ) && defined( __MSDOS__ )

#include <dos.h>
#include <stdlib.h>
#include <string.h>

namespace
{

const unsigned utf8CodePage = 65001;

// Reads 16 bytes at the real mode address seg:off.
static bool readRealMode( unsigned seg, unsigned off, char (&buf)[16] ) noexcept
{
#if defined( __DPMI32__ ) || defined( __DPMI16__ )
    // DPMI function 0002h: a selector for a real mode segment.
    union REGS r;
    r.x.ax = 0x0002;
    r.x.bx = seg;
    int86( 0x31, &r, &r );
    if( r.x.cflag )
        return false;
    const char far *p = (const char far *) MK_FP( r.x.ax, off );
    memcpy( buf, p, 16 );
    return true;
#else
    const char far *p = (const char far *) MK_FP( seg, off );
    memcpy( buf, p, 16 );
    return true;
#endif
}

// Looks for the AMIS provider whose signature (manufacturer and product, 8
// characters each) is sig. Returns the multiplex number or -1.
static int amisFind( const char (&sig)[16] ) noexcept
{
    for( int mux = 0; mux < 256; ++mux )
    {
        union REGS r;
        r.x.ax = (unsigned) mux << 8;       // AH = multiplex number, AL = 0: installation check
        int86( 0x2D, &r, &r );
        if( r.h.al != 0xFF )
            continue;
        char got[16];
        if( readRealMode( r.x.dx, r.x.di, got ) && memcmp( got, sig, 16 ) == 0 )
            return mux;
    }
    return -1;
}

// Function 10h: sets the encoding of this process; function 11h reads it back.
static bool amisSetEncoding( int mux, unsigned encoding ) noexcept
{
    union REGS r;
    r.x.ax = ((unsigned) mux << 8) | 0x10;
    r.x.bx = encoding;
    int86( 0x2D, &r, &r );
    if( r.h.al != 0xFF )
        return false;
    r.x.ax = ((unsigned) mux << 8) | 0x11;
    int86( 0x2D, &r, &r );
    return r.h.al == 0xFF && r.x.bx == encoding;
}

} // namespace

void initDosUtf8() noexcept
{
    static bool done = false;
    if( done )
        return;
    done = true;
    const char *env = getenv( "TV_DOS_UTF8_NAMES" );
    if( env && env[0] == '0' )
        return;
    static const char sig[16] =
        { 'D','O','S','-','U','T','F','8', 'N','A','M','E','S',' ',' ',' ' };
    int mux = amisFind( sig );
    if( mux >= 0 )
        amisSetEncoding( mux, utf8CodePage );
}

#else

void initDosUtf8() noexcept {}

#endif
