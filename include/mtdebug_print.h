#pragma once

#include <Urho3D/Core/Mutex.h>
#include <sstream>
#include <math_utils.h>

#define iout MTDebug_Print(true, false, urho_lt_info)
#define iout_nl MTDebug_Print(true, true, urho_lt_info)
#define iout_ns MTDebug_Print(false, false, urho_lt_info)
#define iout_ns_nl MTDebug_Print(false, true, urho_lt_info)

#define wout MTDebug_Print(true, false, urho_lt_warning)
#define wout_nl MTDebug_Print(true, true, urho_lt_warning)
#define wout_ns MTDebug_Print(false, false, urho_lt_warning)
#define wout_ns_nl MTDebug_Print(false, true, urho_lt_warning)

#define eout MTDebug_Print(true, false, urho_lt_error)
#define eout_nl MTDebug_Print(true, true, urho_lt_error)
#define eout_ns MTDebug_Print(false, false, urho_lt_error)
#define eout_ns_nl MTDebug_Print(false, true, urho_lt_error)

#define dout MTDebug_Print(true, false, urho_lt_debug)
#define dout_nl MTDebug_Print(true, true, urho_lt_debug)
#define dout_ns MTDebug_Print(false, false, urho_lt_debug)
#define dout_ns_nl MTDebug_Print(false, true, urho_lt_debug)

#define tout MTDebug_Print(true, false, urho_lt_trace)
#define tout_nl MTDebug_Print(true, true, urho_lt_trace)
#define tout_ns MTDebug_Print(false, false, urho_lt_trace)
#define tout_ns_nl MTDebug_Print(false, true, urho_lt_trace)

enum urho_log_type
{
    urho_lt_info,
    urho_lt_warning,
    urho_lt_error,
    urho_lt_debug,
    urho_lt_trace
};


class MTDebug_Print
{
public:

    MTDebug_Print(bool auto_spacing, bool auto_newline, urho_log_type logtype);
    ~MTDebug_Print();

    template<class T>
    MTDebug_Print & operator<<(const T & rhs)
    {
        m_mtx.Acquire();
        if (autspc)
            ss << " ";
        ss << rhs;
        if (autonln)
            ss << "\n";
        m_mtx.Release();
        return *this;
    }

private:
    static Urho3D::Mutex m_mtx;
    String str;
    std::stringstream ss;    
    bool autspc;
    bool autonln;
    int lt;
};

