#include <mtdebug_print.h>
#include <Urho3D/IO/Log.h>

using namespace Urho3D;

Mutex MTDebug_Print::m_mtx;

MTDebug_Print::MTDebug_Print(bool auto_spacing, bool auto_newline, urho_log_type logtype):
    str(),
    ss(),
    autspc(auto_spacing),
    autonln(auto_newline),
    lt(logtype)
{
}

MTDebug_Print::~MTDebug_Print()
{
    m_mtx.Acquire();
    String msg(ss.str().c_str());
    switch (lt)
    {
        case(urho_lt_info):
            URHO3D_LOGINFO(msg);
            break;
        case(urho_lt_warning):
            URHO3D_LOGWARNING(msg);
            break;
        case(urho_lt_error):
            URHO3D_LOGERROR(msg);
            break;
        case(urho_lt_debug):
            URHO3D_LOGDEBUG(msg);
            break;
        case(urho_lt_trace):
            URHO3D_LOGTRACE(msg);
            break;
    }
	m_mtx.Release();
}
