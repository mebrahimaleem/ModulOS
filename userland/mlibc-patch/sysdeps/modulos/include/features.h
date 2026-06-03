#ifndef FEATURES_H
#define FEATURES_H

#ifndef __MODULOS__
#define __MODULOS__
#endif /* __MODULOS__ */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif /* _DEFAULT_SOURCE */

#ifndef __unix__
#define __unix__ 1
#endif /* __unix__ */

#ifndef _POSIX_VERSION
#define _POSIX_VERSION 200809L 
#endif /* _POSIX_VERSION */

#ifndef __GLIBC__
#define __GLIBC__ 2
#endif /* __GLIBC__ */

#ifdef __GLIBC_MINOR__
#define __GLIBC_MINOR__ 41
#endif /* __GLIBC_MINOR__ */

#endif /* _FEATURES_H */
