#pragma once

#include <napi.h>

#include "buffer.h"
#include "ygo/common.h"
#include "ygo/ocgapi.h"

using namespace Napi;

bool parseQuery(Env& env, BufferReader& reader, Object& value, bool* eof);

Value parseField(Env& env, BufferReader& reader, bool* eof);
