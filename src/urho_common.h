#pragma once

#include <Urho3D/Urho3D.h>

#include <Urho3D/Math/AreaAllocator.h>
#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Math/Color.h>
#include <Urho3D/Math/Frustum.h>
#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/Math/Matrix2.h>
#include <Urho3D/Math/Matrix3.h>
#include <Urho3D/Math/Matrix3x4.h>
#include <Urho3D/Math/Matrix4.h>
#include <Urho3D/Math/Plane.h>
#include <Urho3D/Math/Polyhedron.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/Random.h>
#include <Urho3D/Math/Ray.h>
#include <Urho3D/Math/Rect.h>
#include <Urho3D/Math/Sphere.h>
#include <Urho3D/Math/StringHash.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Math/Vector4.h>

#include <Urho3D/Container/Allocator.h>
#include <Urho3D/Container/ArrayPtr.h>
#include <Urho3D/Container/FlagSet.h>
#include <Urho3D/Container/ForEach.h>
#include <Urho3D/Container/Hash.h>
#include <Urho3D/Container/HashBase.h>
#include <Urho3D/Container/HashMap.h>
#include <Urho3D/Container/HashSet.h>
#include <Urho3D/Container/LinkedList.h>
#include <Urho3D/Container/List.h>
#include <Urho3D/Container/ListBase.h>
#include <Urho3D/Container/Pair.h>
#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Container/RefCounted.h>
#include <Urho3D/Container/Sort.h>
#include <Urho3D/Container/Str.h>
#include <Urho3D/Container/Swap.h>
#include <Urho3D/Container/Vector.h>
#include <Urho3D/Container/VectorBase.h>

#include <Urho3D/Core/Attribute.h>
#include <Urho3D/Core/Condition.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/EventProfiler.h>
#include <Urho3D/Core/Main.h>
#include <Urho3D/Core/MiniDump.h>
#include <Urho3D/Core/Mutex.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/Core/Spline.h>
#include <Urho3D/Core/StringHashRegister.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Thread.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/Core/Variant.h>
#include <Urho3D/Core/WorkQueue.h>

#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Engine/EngineEvents.h>

#include <mtdebug_print.h>

using namespace Urho3D;

using fvec2 = Vector2;
using fvec3 = Vector3;
using fvec4 = Vector4;
using ivec2 = IntVector2;
using ivec3 = IntVector3;
using irect = IntRect;
using frect = Rect;

using fquat = Quaternion;
using fmat2 = Matrix2;
using fmat3 = Matrix3;
using fmat4 = Matrix4;
using fmat3x4 = Matrix3x4;

using ustr = String;

using uint8 = uint8_t;
using int8 = int8_t;
using uint16 = uint16_t;
using int16 = int16_t;
using uint32 = uint32_t;
using int32 = int32_t;
using uint64 = uint64_t;
using int64 = int64_t;