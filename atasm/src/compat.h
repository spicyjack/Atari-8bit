#ifndef COMPAT_H
#define COMPAT_H

#ifdef MSVC
#define snprintf(buf,sz,format,x) _snprintf(buf,sz,format,x)
#define STRCASECMP(a,b) _stricmp((a),(b))
#define STRNCASECMP(a,b,c) _strnicmp((a),(b),(c))
#else
#define STRCASECMP(a,b) strcasecmp((a),(b))
#define STRNCASECMP(a,b,c) strncasecmp((a),(b),(c))
#endif

#endif
