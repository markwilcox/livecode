/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

// Due to licensing issues with the Inneractive SDK, support for mobile ads
// in community is disabled.

#include "prefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "globals.h"
#include "stack.h"
#include "system.h"
#include "player.h"
#include "eventqueue.h"
#include "osspec.h"

#include "mblandroid.h"
#include "mblandroidutil.h"

#include "mblandroidjava.h"
#include "mblsyntax.h"

#include <jni.h>
#include "mblandroidjava.h"

#include "mblad.h"

#ifdef FEATURE_INNERACTIVE

////////////////////////////////////////////////////////////////////////////////

static jobject s_admodule = nil;

////////////////////////////////////////////////////////////////////////////////

class MCAndroidInneractiveAd : public MCAd
{
public:
    MCAndroidInneractiveAd(MCAdType p_type, MCAdTopLeft p_top_left, uint32_t p_timeout, jobject p_meta_data);
    
    bool Create(void);
    void Delete(void);
    
    jobject GetView(void);
    
    bool GetVisible(void);
    void SetVisible(bool p_visible);
    MCAdTopLeft GetTopLeft();
    void SetTopLeft(MCAdTopLeft p_top_left);
    
    static bool FindByView(jobject p_view, MCAndroidInneractiveAd *&r_ad);   
    
private:
	jobject m_view;
    MCAdType m_type;
    MCAdTopLeft m_top_left;
    uint32_t m_timeout;
    jobject m_meta_data;
};

////////////////////////////////////////////////////////////////////////////////

MCAndroidInneractiveAd::MCAndroidInneractiveAd(MCAdType p_type, MCAdTopLeft p_top_left, uint32_t p_timeout, jobject p_meta_data)
{
    m_view = nil;
    m_type = p_type;
    m_top_left = p_top_left;
    m_timeout = p_timeout;
    m_meta_data = p_meta_data;
}

bool MCAndroidInneractiveAd::Create(void)
{
	MCAndroidObjectRemoteCall(s_admodule, "createInneractiveAd", "osiiiim", &m_view, MCAdGetInneractiveKey(), m_type, m_top_left.x, m_top_left.y, m_timeout, m_meta_data);
    if (m_meta_data != nil)
    {
        JNIEnv *env;
        env = MCJavaGetThreadEnv();        
        MCJavaFreeMap(env, m_meta_data);
        m_meta_data = nil;
    }
    return (m_view != nil);
}

void MCAndroidInneractiveAd::Delete()
{
    JNIEnv *env;
    env = MCJavaGetThreadEnv();    

    if (m_view != nil)
    {
		MCAndroidObjectRemoteCall(s_admodule, "removeAd", "vo", nil, m_view);
        env->DeleteGlobalRef(m_view);        
        m_view = nil;
    }
    
    if (m_meta_data != nil)
    {
        MCJavaFreeMap(env, m_meta_data);
        m_meta_data = nil;
    }
}

jobject MCAndroidInneractiveAd::GetView(void)
{
    return m_view;
}

bool MCAndroidInneractiveAd::GetVisible(void)
{    
    bool t_visible;
    t_visible = false;
    MCAndroidObjectRemoteCall(m_view, "getVisible", "b", &t_visible);
    return t_visible;
}

void MCAndroidInneractiveAd::SetVisible(bool p_visible)
{
    MCAndroidObjectRemoteCall(m_view, "setVisible", "vb", nil, p_visible);
}

MCAdTopLeft MCAndroidInneractiveAd::GetTopLeft()
{   
    MCAdTopLeft t_top_left = {0,0};
    MCAndroidObjectRemoteCall(m_view, "getLeft", "i", &t_top_left.x);
    MCAndroidObjectRemoteCall(m_view, "getTop", "i", &t_top_left.y);
    return t_top_left;     
}

void MCAndroidInneractiveAd::SetTopLeft(MCAdTopLeft p_top_left)
{    
    MCAndroidObjectRemoteCall(m_view, "setTopLeft", "vii", nil, p_top_left.x, p_top_left.y);
}

////////////////////////////////////////////////////////////////////////////////

// TODO - Should probably improve this to use a similar method to native controls.
bool MCAndroidInneractiveAd::FindByView(jobject p_view, MCAndroidInneractiveAd *&r_ad)
{
    JNIEnv *env;
    env = MCJavaGetThreadEnv();
    
    for(MCAndroidInneractiveAd *t_ad = (MCAndroidInneractiveAd*) MCAd::GetFirst(); t_ad != nil; t_ad = (MCAndroidInneractiveAd*) t_ad->GetNext())
        if (env->IsSameObject(p_view, t_ad->GetView()) == JNI_TRUE)
        {
            r_ad = t_ad;
            return true;
        }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

void MCSystemInneractiveAdInit()
{
	s_admodule = nil;
}

//////////

// IM-2013-04-17: [[ AdModule ]] call out to Engine method to load AdModule class
bool MCAndroidInneractiveAdInitModule()
{
	if (s_admodule != nil)
		return true;
	
	MCAndroidEngineCall("loadAdModule", "o", &s_admodule);
	
	return s_admodule != nil;
}

//////////

bool MCSystemInneractiveAdCreate(MCExecContext &ctxt, MCAd *&r_ad, MCAdType p_type, MCAdTopLeft p_top_left, uint32_t p_timeout, MCVariableValue *p_meta_data)
{
	if (!MCAndroidInneractiveAdInitModule())
		return false;
	
    bool t_success;
    t_success = true;
        
    JNIEnv *t_env;
    t_env = MCJavaGetThreadEnv();
    jobject t_meta_data;
    t_meta_data = nil;
    if (t_success)
        t_success = MCJavaInitMap(t_env, t_meta_data);
    
    if (p_meta_data != nil)
    {
        if (t_success)
            if (p_meta_data->fetch_element_if_exists(ctxt.GetEP(), "age", false))
                t_success = MCJavaMapPutStringToString(t_env, t_meta_data, "age", ctxt.GetEP().getcstring());  
        if (t_success)
            if (p_meta_data->fetch_element_if_exists(ctxt.GetEP(), "distribution id", false))
                t_success = MCJavaMapPutStringToString(t_env, t_meta_data, "distribution id", ctxt.GetEP().getcstring());
        /*if (t_success)
            if (p_meta_data->fetch_element_if_exists(ctxt.GetEP(), "external id", false))
                t_success = MCJavaMapPutStringToString(t_env, t_meta_data, "external id", ctxt.GetEP().getcstring());*/
        if (t_success)
            if (p_meta_data->fetch_element_if_exists(ctxt.GetEP(), "gender", false))
                t_success = MCJavaMapPutStringToString(t_env, t_meta_data, "gender", ctxt.GetEP().getcstring());
        if (t_success)
            if (p_meta_data->fetch_element_if_exists(ctxt.GetEP(), "keywords", false))
                t_success = MCJavaMapPutStringToString(t_env, t_meta_data, "keywords", ctxt.GetEP().getcstring());
        if (t_success)
            if (p_meta_data->fetch_element_if_exists(ctxt.GetEP(), "phone number", false))
                t_success = MCJavaMapPutStringToString(t_env, t_meta_data, "phone number", ctxt.GetEP().getcstring());
    }
    
    MCAd *t_ad;
    t_ad = nil;
    if (t_success)
    {
        t_ad = new MCAndroidInneractiveAd(p_type, p_top_left, p_timeout, t_meta_data);
        t_success = t_ad != nil;
    }
    
    if (t_success)
        r_ad = t_ad;
    
    if (!t_success)
        if (t_meta_data != nil)
            MCJavaFreeMap(t_env, t_meta_data);
	
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_InneractiveAdWrapper_doAdUpdate(JNIEnv *env, jobject object, jint event) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_InneractiveAdWrapper_doAdUpdate(JNIEnv *env, jobject object, jint event)
{
    MCAndroidInneractiveAd *t_ad;
    t_ad = nil;
	MCAdPostMessage(t_ad, (MCAdEventType) event);
}

////////////////////////////////////////////////////////////////////////////////

#else

void MCSystemInneractiveAdInit()
{
}

bool MCSystemInneractiveAdCreate(MCExecContext &ctxt, MCAd *&r_ad, MCAdType p_type, MCAdTopLeft p_top_left, uint32_t p_timeout, MCVariableValue *p_meta_data)
{
	return false;
}

#endif

