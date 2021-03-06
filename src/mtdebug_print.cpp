#include <mtdebug_print.h>
#include <Urho3D/IO/Log.h>
#include <iostream>

using namespace Urho3D;

MTDebug_Print::MTDebug_Print(bool auto_spacing, bool auto_newline, Urho_Log_Type logtype)
    : str_(), ss_(), autspc_(auto_spacing), autonln_(auto_newline), lt_(logtype)
{
}

MTDebug_Print::~MTDebug_Print()
{
    String msg(ss_.str().c_str());
    switch (lt_)
    {
    case (URHO_LT_INFO):
        URHO3D_LOGINFO(msg);
        break;
    case (URHO_LT_WARNING):
        URHO3D_LOGWARNING(msg);
        break;
    case (URHO_LT_ERROR):
        URHO3D_LOGERROR(msg);
        break;
    case (URHO_LT_DEBUG):
        URHO3D_LOGDEBUG(msg);
        break;
    case (URHO_LT_TRACE):
        URHO3D_LOGTRACE(msg);
        break;
    }
}
