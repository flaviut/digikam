#ifndef __ExpatAdapter_hpp__
#define __ExpatAdapter_hpp__

// =================================================================================================
// Copyright 2005-2008 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMP_Environment.h"	// ! Must be the first #include!
#include "XMLParserAdapter.hpp"

// =================================================================================================
// Derived XML parser adapter for Expat.
// =================================================================================================
struct XML_ParserStruct;	// ! Hack to avoid exposing expat.h to all clients.
typedef struct XML_ParserStruct *XML_Parser;

namespace DngXmpSdk {
#ifndef BanAllEntityUsage
    #define BanAllEntityUsage	0
#endif



class ExpatAdapter : public XMLParserAdapter {
public:

    XML_Parser parser;

    #if BanAllEntityUsage
        bool isAborted;
    #endif

    #if XMP_DebugBuild
        size_t elemNesting;
    #endif

    ExpatAdapter();
    virtual ~ExpatAdapter();

    void ParseBuffer ( const void * buffer, size_t length, bool last = true );

};

extern "C" ExpatAdapter * XMP_NewExpatAdapter();

// =================================================================================================
} //namespace
#endif	// __ExpatAdapter_hpp__
