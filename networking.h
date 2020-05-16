#ifndef NETWORKING_H
#define NETWORKING_H

#ifdef WINNT
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#ifdef ANDROID
#include <android/multinetwork.h>
#include <linux/in.h>
#else
#include <netdb.h>
#endif
#endif

#endif
