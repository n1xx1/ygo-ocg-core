#pragma once

#include "buffer.h"
#include "ygo/common.h"
#include "ygo/ocgapi.h"
#include <napi.h>

using namespace Napi;

Value readMessage(Env& env, BufferReader& reader, bool* eof);
