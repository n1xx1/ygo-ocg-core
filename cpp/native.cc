#include "buffer.h"
#include "messages.h"
#include "query.h"
#include "ygo/common.h"
#include "ygo/ocgapi.h"
#include <napi.h>
#include <unordered_map>

using namespace Napi;

#define throw_type_error(err)                                                  \
    TypeError::New(env, err).ThrowAsJavaScriptException()

#define throw_error(err) Error::New(env, err).ThrowAsJavaScriptException()

#define jsUndef env.Undefined()

Array ocgGetVersion(const CallbackInfo& info) {
    Env env = info.Env();
    int major, minor;
    OCG_GetVersion(&major, &minor);

    Array ret = Array::New(env, 2);
    ret.Set(uint32(0), Number::New(env, major));
    ret.Set(uint32(1), Number::New(env, minor));
    return ret;
}

struct duelHandle;

struct duelCallback {
    duelCallback(FunctionReference&& ref) : ref(std::move(ref)){};
    duelHandle* handle;
    FunctionReference ref;
};

struct cardDataStorage : OCG_CardData {
    uint16 setcodeptr[4];
};

struct duelHandle {
    OCG_Duel duel;
    std::unordered_map<uint32, cardDataStorage> cards;
    duelCallback* cardReaderPayload;
    duelCallback* scriptReaderPayload;
    duelCallback* logHandlerPayload;
    duelCallback* cardReaderDonePayload;
};

struct instanceData {
    ObjectReference handleSymbol;
};

duelHandle* getDuelHandle(Env& env, Value object) {
    instanceData* instance = env.GetInstanceData<instanceData>();
    Symbol handleSymbol = instance->handleSymbol.Get(uint32(0)).As<Symbol>();
    if (!object.As<Object>().Has(handleSymbol)) {
        return nullptr;
    }
    External<duelHandle> external =
        object.As<Object>().Get(handleSymbol).As<External<duelHandle>>();
    return external.Data();
}

void handleCardReader(void* payload, uint32 code, OCG_CardData* data) {
    duelCallback* handle = static_cast<duelCallback*>(payload);
    FunctionReference& ref = handle->ref;
    Env env = ref.Env();
    Value value = ref.Call({Number::New(env, code)});

    if (!value.IsObject() || value.IsFunction()) {
        throw_type_error("cardReader: should return an object");
        return;
    }

    Object card = value.As<Object>();
    uint32 cardCode = card.Get("code").As<Number>().Int32Value();

    if (handle->handle->cards.count(cardCode) == 1) {
        *data = handle->handle->cards[cardCode];
        return;
    }

    cardDataStorage& cardData = handle->handle->cards[cardCode];
    cardData.setcodes = cardData.setcodeptr;
    cardData.code = cardCode;
    cardData.alias = card.Get("alias").As<Number>().Int32Value();
    cardData.type = card.Get("type").As<Number>().Int32Value();
    cardData.level = card.Get("level").As<Number>().Int32Value();
    cardData.attribute = card.Get("attribute").As<Number>().Int32Value();
    cardData.race = card.Get("race").As<Number>().Int32Value();
    cardData.attack = card.Get("attack").As<Number>().Int32Value();
    cardData.defense = card.Get("defense").As<Number>().Int32Value();
    cardData.lscale = card.Get("lscale").As<Number>().Int32Value();
    cardData.rscale = card.Get("rscale").As<Number>().Int32Value();
    cardData.link_marker = card.Get("link_marker").As<Number>().Int32Value();

    Array setcodes = card.Get("setcodes").As<Array>();
    for (int i = 0, L = setcodes.Length(); i < L; i++) {
        cardData.setcodeptr[i] = setcodes.Get(i).As<Number>().Int32Value();
    }

    *data = cardData;
}

void handleCardReaderDone(void* payload, OCG_CardData* data) {
    duelCallback* handle = static_cast<duelCallback*>(payload);
    FunctionReference& ref = handle->ref;
    // TODO: implement
}

int handleScriptReader(void* payload, OCG_Duel duel, const char* name) {
    duelCallback* handle = static_cast<duelCallback*>(payload);
    FunctionReference& ref = handle->ref;
    Env env = ref.Env();
    Value res = ref.Call(ref.Env().Undefined(), {String::New(ref.Env(), name)});
    return res.ToBoolean();
}

void handleLogHandler(void* payload, const char* string, int type) {
    duelCallback* handle = static_cast<duelCallback*>(payload);
    FunctionReference& ref = handle->ref;
    Env env = ref.Env();
    ref.Call({String::New(env, string), Number::New(env, type)});
}

Value ocgCreateDuel(const CallbackInfo& info) {
    Env env = info.Env();
    OCG_DuelOptions options = {};
    Object argConfig = info[0].As<Object>();

    bool loseless;
    options.flags = argConfig.Get("flags").As<BigInt>().Uint64Value(&loseless);
    options.seed = argConfig.Get("seed").As<Number>().Int64Value();
    options.team1.drawCountPerTurn =
        argConfig.Get("team1_drawCountPerTurn").As<Number>().Int32Value();
    options.team1.startingDrawCount =
        argConfig.Get("team1_startingDrawCount").As<Number>().Int32Value();
    options.team1.startingLP =
        argConfig.Get("team1_startingLP").As<Number>().Int32Value();
    options.team2.drawCountPerTurn =
        argConfig.Get("team2_drawCountPerTurn").As<Number>().Int32Value();
    options.team2.startingDrawCount =
        argConfig.Get("team2_startingDrawCount").As<Number>().Int32Value();
    options.team2.startingLP =
        argConfig.Get("team2_startingLP").As<Number>().Int32Value();

    options.cardReader = handleCardReader;
    options.payload1 = new duelCallback(
        Persistent(argConfig.Get("cardReader").As<Function>()));

    options.scriptReader = handleScriptReader;
    options.payload2 = new duelCallback(
        Persistent(argConfig.Get("scriptReader").As<Function>()));

    if (!argConfig.Get("logHandler").IsUndefined()) {
        options.logHandler = handleLogHandler;
        options.payload3 = new duelCallback(
            Persistent(argConfig.Get("logHandler").As<Function>()));
    }
    if (!argConfig.Get("cardReaderDone").IsUndefined()) {
        options.cardReaderDone = handleCardReaderDone;
        options.payload4 = new duelCallback(
            Persistent(argConfig.Get("cardReaderDone").As<Function>()));
    }

    OCG_Duel duel;
    int result = OCG_CreateDuel(&duel, options);

    if (result != OCG_DUEL_CREATION_SUCCESS) {
        switch (result) {
        case OCG_DUEL_CREATION_NO_OUTPUT:
            throw_error("createDuel: NO_OUTPUT");
            return jsUndef;
        case OCG_DUEL_CREATION_NOT_CREATED:
            throw_error("createDuel: NOT_CREATED");
            return jsUndef;
        default:
            throw_error("createDuel: NOT_CREATED");
            return jsUndef;
        }
    }

    duelHandle* handle = new duelHandle{};
    handle->duel = duel;

    handle->cardReaderPayload = (duelCallback*)options.payload1;
    handle->cardReaderPayload->handle = handle;

    handle->scriptReaderPayload = (duelCallback*)options.payload2;
    handle->cardReaderPayload->handle = handle;

    if (options.payload3) {
        handle->logHandlerPayload = (duelCallback*)options.payload3;
        handle->cardReaderPayload->handle = handle;
    }
    if (options.payload4) {
        handle->cardReaderDonePayload = (duelCallback*)options.payload4;
        handle->cardReaderPayload->handle = handle;
    }

    instanceData* instance = env.GetInstanceData<instanceData>();
    Symbol handleSymbol = instance->handleSymbol.Get(uint32(0)).As<Symbol>();

    Object duelHandleObject = Object::New(env);
    duelHandleObject.DefineProperties({PropertyDescriptor::Value(
        handleSymbol,
        External<duelHandle>::New(env, handle, [](Env env, duelHandle* handle) {
            OCG_DestroyDuel(handle->duel);
            if (handle->cardReaderPayload)
                delete handle->cardReaderPayload;
            if (handle->scriptReaderPayload)
                delete handle->scriptReaderPayload;
            if (handle->logHandlerPayload)
                delete handle->logHandlerPayload;
            if (handle->cardReaderDonePayload)
                delete handle->cardReaderDonePayload;
            delete handle;
        }))});

    return duelHandleObject;
}

Value ocgLoadScript(const CallbackInfo& info) {
    Env env = info.Env();

    duelHandle* handle = getDuelHandle(env, info[0]);
    if (handle == nullptr) {
        throw_type_error(
            "loadScript: \"duel\" has wrong type, duel handle expected");
        return env.Undefined();
    }

    std::string buffer = info[1].As<String>().Utf8Value();
    std::string name = info[2].As<String>().Utf8Value();

    int result = OCG_LoadScript(handle->duel, buffer.data(), buffer.length(),
                                name.c_str());
    return Boolean::New(env, result == 1);
}

Value ocgDuelNewCard(const CallbackInfo& info) {
    Env env = info.Env();

    duelHandle* handle = getDuelHandle(env, info[0]);
    if (handle == nullptr) {
        throw_type_error(
            "duelNewCard: \"duel\" has wrong type, duel handle expected");
        return env.Undefined();
    }

    OCG_NewCardInfo cardData = {};

    Object card = info[1].As<Object>();
    cardData.team = card.Get("team").As<Number>().Int32Value();
    cardData.duelist = card.Get("duelist").As<Number>().Int32Value();
    cardData.code = card.Get("code").As<Number>().Int32Value();
    cardData.con = card.Get("con").As<Number>().Int32Value();
    cardData.loc = card.Get("loc").As<Number>().Int32Value();
    cardData.seq = card.Get("seq").As<Number>().Int32Value();
    cardData.pos = card.Get("pos").As<Number>().Int32Value();

    OCG_DuelNewCard(handle->duel, cardData);
    return env.Undefined();
}

Value ocgStartDuel(const CallbackInfo& info) {
    Env env = info.Env();

    duelHandle* handle = getDuelHandle(env, info[0]);
    if (handle == nullptr) {
        throw_type_error(
            "startDuel: \"duel\" has wrong type, duel handle expected");
        return env.Undefined();
    }

    OCG_StartDuel(handle->duel);
    return env.Undefined();
}

Value ocgDuelProcess(const CallbackInfo& info) {
    Env env = info.Env();

    duelHandle* handle = getDuelHandle(env, info[0]);
    if (handle == nullptr) {
        throw_type_error(
            "startDuel: \"duel\" has wrong type, duel handle expected");
        return env.Undefined();
    }

    return Number::New(env, OCG_DuelProcess(handle->duel));
}

Value ocgDuelGetMessage(const CallbackInfo& info) {
    Env env = info.Env();

    duelHandle* handle = getDuelHandle(env, info[0]);
    if (handle == nullptr) {
        throw_type_error(
            "duelGetMessage: \"duel\" has wrong type, duel handle expected");
        return env.Undefined();
    }

    Array messageList = Array::New(env);
    uint32 index = 0;

    uint32 bufferLength;
    void* buffer = OCG_DuelGetMessage(handle->duel, &bufferLength);
    BufferReader reader{buffer, bufferLength};
    bool eof = false;
    while (true) {
        uint32 messageLength = reader.read<uint32>(&eof);
        if (eof)
            break;

        BufferReader subReader = reader.subReader(messageLength, &eof);
        if (eof) {
            throw_error("duelGetMessage: eof while reading messages");
            return env.Undefined();
        }
        Value message = readMessage(env, subReader, &eof);
        if (eof) {
            throw_error("duelGetMessage: eof while reading messages");
            return env.Undefined();
        }
        messageList.Set(index, message);
        index++;
    }
    return messageList;
}

Value ocgDuelQueryCount(const CallbackInfo& info) {
    Env env = info.Env();

    duelHandle* handle = getDuelHandle(env, info[0]);
    if (handle == nullptr) {
        throw_type_error(
            "duelGetMessage: \"duel\" has wrong type, duel handle expected");
        return env.Undefined();
    }

    uint8_t team = info[1].As<Number>().Uint32Value();
    uint32 location = info[2].As<Number>().Uint32Value();
    uint32 count = OCG_DuelQueryCount(handle->duel, team, location);
    return Number::New(env, count);
}

const uint32 defaultQueryFlags =
    QUERY_CODE | QUERY_POSITION | QUERY_ALIAS | QUERY_TYPE | QUERY_LEVEL |
    QUERY_RANK | QUERY_ATTRIBUTE | QUERY_RACE | QUERY_ATTACK | QUERY_DEFENSE |
    QUERY_BASE_ATTACK | QUERY_BASE_DEFENSE | QUERY_REASON | QUERY_REASON_CARD |
    QUERY_EQUIP_CARD | QUERY_TARGET_CARD | QUERY_OVERLAY_CARD | QUERY_COUNTERS |
    QUERY_OWNER | QUERY_STATUS | QUERY_IS_PUBLIC | QUERY_LSCALE | QUERY_RSCALE |
    QUERY_LINK | QUERY_IS_HIDDEN | QUERY_COVER;

Value ocgDuelQuery(const CallbackInfo& info) {
    Env env = info.Env();

    duelHandle* handle = getDuelHandle(env, info[0]);
    if (handle == nullptr) {
        throw_type_error(
            "duelQuery: \"duel\" has wrong type, duel handle expected");
        return env.Undefined();
    }

    OCG_QueryInfo queryInfo = {};
    queryInfo.flags = defaultQueryFlags;
    queryInfo.con = info[1].As<Number>().Uint32Value();
    queryInfo.loc = info[2].As<Number>().Uint32Value();
    queryInfo.seq = info[3].As<Number>().Uint32Value();
    if (!info[4].IsUndefined()) {
        queryInfo.overlay_seq = info[4].As<Number>().Uint32Value();
    }

    uint32 bufferLength;
    void* buffer = OCG_DuelQuery(handle->duel, &bufferLength, queryInfo);
    BufferReader reader{buffer, bufferLength};

    Object ret = Object::New(env);
    bool eof = false;
    while (true) {
        uint16 queryLength = reader.read<uint16>(&eof);
        if (eof)
            break;

        BufferReader subReader = reader.subReader(queryLength, &eof);
        if (eof) {
            throw_error("duelQuery: eof while reading queries");
            return env.Undefined();
        }
        parseQuery(env, subReader, ret, &eof);
        if (eof) {
            throw_error("duelQuery: eof while reading queries");
            return env.Undefined();
        }
    }
    return ret;
}

Value ocgDuelQueryLocation(const CallbackInfo& info) {
    Env env = info.Env();

    duelHandle* handle = getDuelHandle(env, info[0]);
    if (handle == nullptr) {
        throw_type_error(
            "duelQueryLocation: \"duel\" has wrong type, duel handle expected");
        return env.Undefined();
    }

    OCG_QueryInfo queryInfo;
    queryInfo.flags = defaultQueryFlags;
    queryInfo.con = info[1].As<Number>().Uint32Value();
    queryInfo.loc = info[2].As<Number>().Uint32Value();

    uint32 bufferLength;
    void* buffer =
        OCG_DuelQueryLocation(handle->duel, &bufferLength, queryInfo);
    BufferReader reader{buffer, bufferLength};
    bool eof = false;

    Array ret = Array::New(env);
    uint32 index = 0;

    uint32 totalSize = reader.read<uint32>(&eof);
    while (reader.avail() > 0) {
        Object value = Object::New(env);

        while (true) {
            uint16 queryLength = reader.read<uint16>(&eof);
            if (eof)
                break;

            BufferReader subReader = reader.subReader(queryLength, &eof);
            if (eof) {
                throw_error("duelQueryLocation: eof while reading queries");
                return env.Undefined();
            }
            bool end = parseQuery(env, subReader, value, &eof);
            if (eof) {
                throw_error("duelQueryLocation: eof while reading queries");
                return env.Undefined();
            }
            if (end) {
                break;
            }
        }
        ret.Set(index++, value);
    }

    return ret;
}

Value ocgDuelQueryField(const CallbackInfo& info) {
    Env env = info.Env();

    duelHandle* handle = getDuelHandle(env, info[0]);
    if (handle == nullptr) {
        throw_type_error(
            "duelQueryField: \"duel\" has wrong type, duel handle expected");
        return env.Undefined();
    }

    uint32 bufferLength;
    void* buffer = OCG_DuelQueryField(handle->duel, &bufferLength);
    BufferReader reader{buffer, bufferLength};
    bool eof = false;

    Value ret = parseField(env, reader, &eof);
    if (eof) {
        throw_error("duelQueryField: eof while reading field");
        return env.Undefined();
    }

    return ret;
}

Object Init(Env env, Object exports) {
    instanceData* instance = new instanceData;
    Object handleSymbol = Object::New(env);
    handleSymbol.Set(uint32(0), Symbol::New(env, "DuelHandle"));
    instance->handleSymbol = Persistent(handleSymbol);

    env.SetInstanceData(instance);

    exports["getVersion"] = Function::New(env, ocgGetVersion);
    exports["createDuel"] = Function::New(env, ocgCreateDuel);
    exports["loadScript"] = Function::New(env, ocgLoadScript);
    exports["duelNewCard"] = Function::New(env, ocgDuelNewCard);
    exports["startDuel"] = Function::New(env, ocgStartDuel);
    exports["duelProcess"] = Function::New(env, ocgDuelProcess);
    exports["duelGetMessage"] = Function::New(env, ocgDuelGetMessage);
    exports["duelQueryCount"] = Function::New(env, ocgDuelQueryCount);
    exports["duelQuery"] = Function::New(env, ocgDuelQuery);
    exports["duelQueryLocation"] = Function::New(env, ocgDuelQueryLocation);
    exports["duelQueryField"] = Function::New(env, ocgDuelQueryField);

    return exports;
}

NODE_API_MODULE(addon, Init)
