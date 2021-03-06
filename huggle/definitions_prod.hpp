//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//! This file exist only for compiler options that can be changed before you build huggle
//! Please do not commit any changes in this file

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

typedef char byte_ht;

#define HUGGLE_VERSION                  "3.1.7"
#define HUGGLE_BYTE_VERSION_MAJOR       0x3
#define HUGGLE_BYTE_VERSION_MINOR       0x1
#define HUGGLE_BYTE_VERSION_RELEASE     0x7
// format is 0xMAJOR(2)MINOR(2)RELEASE(2) so for 3.1.2 it's 0x030102
#define HUGGLE_BYTE_VERSION             0x030107

// Version of mediawiki that we do support
#define HUGGLE_SUPPORTED_MEDIAWIKI_VERSION "1.25"

// How often the statistics get purged in seconds (smaller, more precise they will be)
#define HUGGLE_STATISTICS_LIFETIME     200
#define HUGGLE_STATISTICS_BLOCK_SIZE   20

// we are using translatewiki and if this is not defined there is a huge overhead of Qt code
#ifndef QT_NO_TRANSLATION
    #define QT_NO_TRANSLATION
#endif

#if defined _WIN64 || defined _WIN32
    #define HUGGLE_WIN
#endif

// comment this out to disable multithreaded garbage collector
// this can be useful for debugging as multithreaded GC is not able to delete Qt objects, so if your code
// is crashing with it only, it means your code suck and need a fix in destructor :))
#ifdef __APPLE__
    #include <cstddef>
    #include "TargetConditionals.h"
    #ifdef TARGET_OS_MAC
// fixme
// this is needed on mac, who knows why, gets a cookie :o
namespace std { typedef decltype(nullptr) nullptr_t; }
        #define HUGGLE_MACX true
        #define HUGGLE_NO_MT_GC
    #endif
#endif
// this is a nasty hack that will disable multi threaded gc on MacOS as we had some report that
// it has problems there (need to be fixed though)
#ifndef HUGGLE_NO_MT_GC
    #define HUGGLE_USE_MT_GC               "mt"
#endif

// #define HUGGLE_PROFILING

// uncomment this if you want to enable python support
#ifndef HUGGLE_PYTHON
//    #define HUGGLE_PYTHON
#endif

// Uncomment this in order to disable breakpad, this is useful when you are having troubles
// linking or building its libraries
#define DISABLE_BREAKPAD

// this is a nasty workaround that exist because python is written by noobs
#ifdef HUGGLE_PYTHON
  #ifdef HUGGLE_WIN
  // workaround for http://bugs.python.org/issue11566
  // remove at least 8 months after the bug is fixed
  #include <cmath>
  #endif
#endif

#ifdef HUGGLE_PYTHON
    #include <Python.h>
#endif

#define HUGGLE_SUCCESS                     1
#define PRODUCTION_BUILD                   0
#define MEDIAWIKI_DEFAULT_NS_MAIN               ""
#define MEDIAWIKI_DEFAULT_NS_TALK               "Talk:"
#define MEDIAWIKI_DEFAULT_NS_USER               "User:"
#define MEDIAWIKI_DEFAULT_NS_USERTALK           "User talk:"
#define MEDIAWIKI_DEFAULT_NS_PROJECT            "Project:"
#define MEDIAWIKI_DEFAULT_NS_PROJECTTALK        "Project talk:"
#define MEDIAWIKI_DEFAULT_NS_FILE               "File:"
#define MEDIAWIKI_DEFAULT_NS_FILETALK           "File talk:"
#define MEDIAWIKI_DEFAULT_NS_MEDIAWIKI          "Mediawiki:"
#define MEDIAWIKI_DEFAULT_NS_MEDIAWIKITALK      "Mediawiki talk:"
#define MEDIAWIKI_DEFAULT_NS_TEMPLATE           "Template:"
#define MEDIAWIKI_DEFAULT_NS_TEMPLATETALK       "Template talk:"
#define MEDIAWIKI_DEFAULT_NS_HELP               "Help:"
#define MEDIAWIKI_DEFAULT_NS_HELPTALK           "Help talk:"
#define MEDIAWIKI_DEFAULT_NS_CATEGORY           "Category:"
#define MEDIAWIKI_DEFAULT_NS_CATEGORYTALK       "Category talk:"
//! Minimal score the edit can have
#define MINIMAL_SCORE                   -999999
#define HUGGLE_CONF                     "huggle3.xml"
//! Path where the extensions are located
#define EXTENSION_PATH                  "extensions"
//! Value that is used by default for timers that are used on various places
//! lower this is, more your CPU will work but faster the huggle will be
#ifndef HUGGLE_TIMER
    #define HUGGLE_TIMER                   200
#endif

#ifndef HUGGLE_EX
    #ifdef HUGGLE_EXTENSION
        #ifdef HUGGLE_WIN
            #define HUGGLE_EX __declspec(dllimport)
        #endif
    #endif
#endif

#ifndef HUGGLE_EX
    #ifdef HUGGLE_LIBRARY 
        #ifdef HUGGLE_WIN
            #define HUGGLE_EX __declspec(dllexport)
        #endif
    #endif
#endif

#ifndef HUGGLE_EX
    #define HUGGLE_EX
#endif

//! Change this to DEBIAN / UBUNTU / WINDOWS to get automatic updates for selected channels
#ifdef __linux__
    #define HUGGLE_UPDATER_PLATFORM_TYPE            "linux"
    #define HUGGLE_GLOBAL_EXTENSION_PATH            "/usr/share/huggle/extensions"
#elif defined HUGGLE_WIN
    #define HUGGLE_UPDATER_PLATFORM_TYPE            "windows"
    #define HUGGLE_GLOBAL_EXTENSION_PATH            QCoreApplication::applicationDirPath() + "\\extensions"
#elif defined HUGGLE_MACX
    #define HUGGLE_UPDATER_PLATFORM_TYPE            "mac"
#else
    #define HUGGLE_UPDATER_PLATFORM_TYPE            "unknown"
#endif

// stolen from boost/current_function.hpp
// enhanced by Petr Bena, keep the enhancement in case you wanted to update this

#ifndef __LINE__
    #define __LINE__ 0
#endif

#ifndef __FILE__
    #define __FILE__ "{unknown}"
#endif

#define HUGGLE_FILE_LINE QString("@") + QString(__FILE__) + QString(":") + QString::number(__LINE__)

#ifndef BOOST_CURRENT_FUNCTION
    #if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600))
        #define BOOST_CURRENT_FUNCTION QString(__PRETTY_FUNCTION__) + HUGGLE_FILE_LINE
    #elif defined(__DMC__) && (__DMC__ >= 0x810)
        #define BOOST_CURRENT_FUNCTION QString(__PRETTY_FUNCTION__) + HUGGLE_FILE_LINE
    #elif defined(__FUNCSIG__)
        #define BOOST_CURRENT_FUNCTION QString(__FUNCSIG__) + HUGGLE_FILE_LINE
    #elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
        #define BOOST_CURRENT_FUNCTION QString(__FUNCTION__) + HUGGLE_FILE_LINE
    #elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
        #define BOOST_CURRENT_FUNCTION QString(__FUNC__) + HUGGLE_FILE_LINE
    #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
        #define BOOST_CURRENT_FUNCTION QString(__func__) + HUGGLE_FILE_LINE
    #else
        #define BOOST_CURRENT_FUNCTION QString("(unknown)") + HUGGLE_FILE_LINE
    #endif
#endif

//! Revid of edit that doesn't exist
#define WIKI_UNKNOWN_REVID -1

#endif // CONFIG_H
