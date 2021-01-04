#include "buffer.h"
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
    ret.Set(uint32_t(0), Number::New(env, major));
    ret.Set(uint32_t(1), Number::New(env, minor));
    return ret;
}

struct duelHandle;

struct duelCallback {
    duelCallback(FunctionReference&& ref) : ref(std::move(ref)){};
    duelHandle* handle;
    FunctionReference ref;
};

struct cardDataStorage : OCG_CardData {
    uint16_t setcodeptr[4];
};

struct duelHandle {
    OCG_Duel duel;
    std::unordered_map<uint32_t, cardDataStorage> cards;
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
    Symbol handleSymbol = instance->handleSymbol.Get(uint32_t(0)).As<Symbol>();
    if (!object.As<Object>().Has(handleSymbol)) {
        return nullptr;
    }
    External<duelHandle> external =
        object.As<Object>().Get(handleSymbol).As<External<duelHandle>>();
    return external.Data();
}

void handleCardReader(void* payload, uint32_t code, OCG_CardData* data) {
    duelCallback* handle = static_cast<duelCallback*>(payload);
    FunctionReference& ref = handle->ref;
    Env env = ref.Env();
    Value value = ref.Call({Number::New(env, code)});

    if (!value.IsObject() || value.IsFunction()) {
        throw_type_error("cardReader: should return an object");
        return;
    }

    Object card = value.As<Object>();
    uint32_t cardCode = card.Get("code").As<Number>().Int32Value();

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
        argConfig.Get("team1_drawCountPerTurn").As<Number>().Int32Value();
    options.team1.startingLP =
        argConfig.Get("team1_startingLP").As<Number>().Int32Value();
    options.team2.drawCountPerTurn =
        argConfig.Get("team2_drawCountPerTurn").As<Number>().Int32Value();
    options.team2.startingDrawCount =
        argConfig.Get("team2_drawCountPerTurn").As<Number>().Int32Value();
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
    Symbol handleSymbol = instance->handleSymbol.Get(uint32_t(0)).As<Symbol>();

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

Value readMessage(Env& env, BufferReader& reader, bool* eof);

Value ocgDuelGetMessage(const CallbackInfo& info) {
    Env env = info.Env();

    duelHandle* handle = getDuelHandle(env, info[0]);
    if (handle == nullptr) {
        throw_type_error(
            "duelGetMessage: \"duel\" has wrong type, duel handle expected");
        return env.Undefined();
    }

    Array messageList = Array::New(env);
    uint32_t index = 0;

    uint32_t bufferLength;
    void* buffer = OCG_DuelGetMessage(handle->duel, &bufferLength);
    BufferReader reader{buffer, bufferLength};
    bool eof = false;
    while (true) {
        uint32_t messageLength = reader.read<uint32_t>(&eof);
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

Object Init(Env env, Object exports) {
    instanceData* instance = new instanceData;
    Object handleSymbol = Object::New(env);
    handleSymbol.Set(uint32_t(0), Symbol::New(env, "DuelHandle"));
    instance->handleSymbol = Persistent(handleSymbol);

    env.SetInstanceData(instance);

    exports["getVersion"] = Function::New(env, ocgGetVersion);
    exports["createDuel"] = Function::New(env, ocgCreateDuel);
    exports["loadScript"] = Function::New(env, ocgLoadScript);
    exports["duelNewCard"] = Function::New(env, ocgDuelNewCard);
    exports["startDuel"] = Function::New(env, ocgStartDuel);
    exports["duelProcess"] = Function::New(env, ocgDuelProcess);
    exports["duelGetMessage"] = Function::New(env, ocgDuelGetMessage);

    return exports;
}

#define READ_VALUE(name, type)                                                 \
    type name = reader.read<type>(eof);                                        \
    do {                                                                       \
        if (*eof) {                                                            \
            printf("debug: eof reading \"%s\" in message %d\n", #name,         \
                   messageType);                                               \
            return {};                                                         \
        }                                                                      \
    } while (0)

#define READ_VALUE_SET(name, type, jstype, jstarget)                           \
    type name = reader.read<type>(eof);                                        \
    do {                                                                       \
        if (*eof) {                                                            \
            printf("debug: eof reading \"%s\" in message %d\n", #name,         \
                   messageType);                                               \
            return {};                                                         \
        }                                                                      \
        jstarget.Set(#name, jstype::New(env, name));                           \
    } while (0)

#define READ_VALUE_VSET(name, type, jsvalue, jstarget)                         \
    type name = reader.read<type>(eof);                                        \
    do {                                                                       \
        if (*eof) {                                                            \
            printf("debug: eof reading \"%s\" in message %d\n", #name,         \
                   messageType);                                               \
            return {};                                                         \
        }                                                                      \
        jstarget.Set(#name, jsvalue);                                          \
    } while (0)

Value readMessage(Env& env, BufferReader& reader, bool* eof) {
    READ_VALUE(messageType, uint8_t);

    Object message = Object::New(env);
    message.Set("type", Number::New(env, messageType));

    switch (messageType) {
    case MSG_HINT: {
        READ_VALUE_SET(hint_type, uint8_t, Number, message);
        READ_VALUE_SET(player, uint8_t, Number, message);
        if (reader.avail() > 4) {
            READ_VALUE_SET(hint, uint64_t, BigInt, message);
        } else {
            READ_VALUE_VSET(hint, uint32_t, BigInt::New(env, uint64_t(hint)),
                            message);
        }
        break;
    }
    case MSG_WIN: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(reason, uint8_t, Number, message);
        break;
    }
    case MSG_SELECT_BATTLECMD: {
        READ_VALUE_SET(player, uint8_t, Number, message);

        READ_VALUE(chainLength, uint32_t);
        Array chains = Array::New(env, chainLength);
        for (uint32_t i = 0; i < chainLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            READ_VALUE_SET(description, uint64_t, Number, value);
            READ_VALUE_SET(client_mode, uint8_t, Number, value);
            chains.Set(i, value);
        }
        message.Set("chains", chains);

        READ_VALUE(attackLength, uint32_t);
        Array attacks = Array::New(env, attackLength);
        for (uint32_t i = 0; i < attackLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint8_t, Number, value);
            READ_VALUE_SET(can_direct, uint8_t, Boolean, value);
            attacks.Set(i, value);
        }
        message.Set("attacks", attacks);

        READ_VALUE_SET(to_m2, uint8_t, Boolean, message);
        READ_VALUE_SET(to_ep, uint8_t, Boolean, message);
        break;
    }
    case MSG_SELECT_IDLECMD: {
        READ_VALUE_SET(player, uint8_t, Number, message);

        READ_VALUE(summonLength, uint32_t);
        Array summons = Array::New(env, summonLength);
        for (uint32_t i = 0; i < summonLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            summons.Set(i, value);
        }
        message.Set("summons", summons);

        READ_VALUE(spSummonLength, uint32_t);
        Array spSummons = Array::New(env, spSummonLength);
        for (uint32_t i = 0; i < spSummonLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            spSummons.Set(i, value);
        }
        message.Set("sp_summons", spSummons);

        READ_VALUE(posChangeLength, uint32_t);
        Array posChanges = Array::New(env, posChangeLength);
        for (uint32_t i = 0; i < posChangeLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint8_t, Number, value);
            posChanges.Set(i, value);
        }
        message.Set("pos_changes", posChanges);

        READ_VALUE(monsterSetLength, uint32_t);
        Array monsterSets = Array::New(env, monsterSetLength);
        for (uint32_t i = 0; i < monsterSetLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            monsterSets.Set(i, value);
        }
        message.Set("monster_sets", monsterSets);

        READ_VALUE(spellSetLength, uint32_t);
        Array spellSets = Array::New(env, spellSetLength);
        for (uint32_t i = 0; i < spellSetLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            spellSets.Set(i, value);
        }
        message.Set("spell_sets", spellSets);

        READ_VALUE(activateLength, uint32_t);
        Array activates = Array::New(env, activateLength);
        for (uint32_t i = 0; i < activateLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            READ_VALUE_SET(description, uint64_t, BigInt, value);
            READ_VALUE_SET(client_mode, uint8_t, Number, value);
            activates.Set(i, value);
        }
        message.Set("activates", activates);

        READ_VALUE_SET(to_bp, uint8_t, Boolean, message);
        READ_VALUE_SET(to_ep, uint8_t, Boolean, message);
        READ_VALUE_SET(shuffle, uint8_t, Boolean, message);
        break;
    }
    case MSG_SELECT_EFFECTYN: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(code, uint32_t, Number, message);
        READ_VALUE_SET(controller, uint8_t, Number, message);
        READ_VALUE_SET(location, uint8_t, Number, message);
        READ_VALUE_SET(sequence, uint32_t, Number, message);
        READ_VALUE_SET(position, uint32_t, Number, message);
        READ_VALUE_SET(description, uint64_t, BigInt, message);
        break;
    }
    case MSG_SELECT_YESNO: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(description, uint64_t, BigInt, message);
        break;
    }
    case MSG_SELECT_OPTION: {
        READ_VALUE_SET(player, uint8_t, Number, message);

        READ_VALUE(optionsLength, uint8_t);
        Array options = Array::New(env, optionsLength);
        for (uint8_t i = 0; i < optionsLength; i++) {
            READ_VALUE(option, uint64_t);
            options.Set(uint32_t(i), BigInt::New(env, option));
        }
        message.Set("options", options);
        break;
    }
    case MSG_SELECT_CARD: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(can_cancel, uint8_t, Boolean, message);
        READ_VALUE_SET(min, uint32_t, Number, message);
        READ_VALUE_SET(max, uint32_t, Number, message);

        READ_VALUE(selectsLength, uint32_t);
        Array selects = Array::New(env, selectsLength);
        for (uint32_t i = 0; i < selectsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            READ_VALUE_SET(position, uint32_t, Number, value);
            selects.Set(i, value);
        }
        message.Set("selects", selects);

        break;
    }
    case MSG_SELECT_CHAIN: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(spe_count, uint8_t, Number, message);
        READ_VALUE_SET(forced, uint8_t, Boolean, message);
        READ_VALUE_SET(hint_timing, uint32_t, Number, message);
        READ_VALUE_SET(hint_timing_other, uint32_t, Number, message);

        READ_VALUE(selectsLength, uint32_t);
        Array selects = Array::New(env, selectsLength);
        for (uint32_t i = 0; i < selectsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            READ_VALUE_SET(position, uint32_t, Number, value);
            READ_VALUE_SET(description, uint64_t, BigInt, value);
            READ_VALUE_SET(client_mode, uint8_t, Number, value);
            selects.Set(i, value);
        }
        message.Set("selects", selects);
        break;
    }
    case MSG_SELECT_PLACE: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(count, uint8_t, Number, message);
        READ_VALUE_SET(field_mask, uint32_t, Number, message);
        break;
    }
    case MSG_SELECT_POSITION: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(code, uint32_t, Number, message);
        READ_VALUE_SET(positions, uint8_t, Number, message);
        break;
    }
    case MSG_SELECT_TRIBUTE: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(can_cancel, uint8_t, Boolean, message);
        READ_VALUE_SET(min, uint32_t, Number, message);
        READ_VALUE_SET(max, uint32_t, Number, message);

        READ_VALUE(selectsLength, uint32_t);
        Array selects = Array::New(env, selectsLength);
        for (uint32_t i = 0; i < selectsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            READ_VALUE_SET(release_param, uint8_t, Number, value);
            selects.Set(i, value);
        }
        message.Set("selects", selects);
        break;
    }
    case MSG_SORT_CHAIN: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE(cardsLength, uint32_t);
        Array cards = Array::New(env, cardsLength);
        for (uint32_t i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint32_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_SELECT_COUNTER: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(counter_type, uint16_t, Number, message);
        READ_VALUE_SET(count, uint16_t, Number, message);
        READ_VALUE(cardsLength, uint32_t);
        Array cards = Array::New(env, cardsLength);
        for (uint32_t i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint8_t, Number, value);
            READ_VALUE_SET(count, uint16_t, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_SELECT_SUM: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(select_max, uint8_t, Boolean, message);
        READ_VALUE_SET(amount, uint32_t, Number, message);
        READ_VALUE_SET(min, uint32_t, Number, message);
        READ_VALUE_SET(max, uint32_t, Number, message);
        READ_VALUE(selectsMustLength, uint32_t);
        Array selectsMust = Array::New(env, selectsMustLength);
        for (uint32_t i = 0; i < selectsMustLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            READ_VALUE_SET(amount, uint32_t, Number, value);
            selectsMust.Set(i, value);
        }
        message.Set("selects_must", selectsMust);

        READ_VALUE(selectLength, uint32_t);
        Array select = Array::New(env, selectLength);
        for (uint32_t i = 0; i < selectLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            READ_VALUE_SET(amount, uint32_t, Number, value);
            select.Set(i, value);
        }
        message.Set("selects", select);
        break;
    }
    case MSG_SELECT_DISFIELD: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(count, uint8_t, Number, message);
        READ_VALUE_SET(field_mask, uint32_t, Number, message);
        break;
    }
    case MSG_SORT_CARD: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE(cardsLength, uint32_t);
        Array cards = Array::New(env, cardsLength);
        for (uint32_t i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint32_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_SELECT_UNSELECT_CARD: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(can_finish, uint8_t, Boolean, message);
        READ_VALUE_SET(can_cancel, uint8_t, Boolean, message);
        READ_VALUE_SET(min, uint32_t, Boolean, message);
        READ_VALUE_SET(max, uint32_t, Boolean, message);

        READ_VALUE(selectCardsLength, uint32_t);
        Array selectCards = Array::New(env, selectCardsLength);
        for (uint32_t i = 0; i < selectCardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            READ_VALUE_SET(position, uint32_t, Number, value);
            selectCards.Set(i, value);
        }
        message.Set("select_cards", selectCards);

        READ_VALUE(unselectCardsLength, uint32_t);
        Array unselectCards = Array::New(env, unselectCardsLength);
        for (uint32_t i = 0; i < unselectCardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            READ_VALUE_SET(position, uint32_t, Number, value);
            unselectCards.Set(i, value);
        }
        message.Set("unselect_cards", unselectCards);

        break;
    }
    case MSG_CONFIRM_DECKTOP: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE(cardsLength, uint32_t);
        Array cards = Array::New(env, cardsLength);
        for (uint32_t i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_CONFIRM_CARDS: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE(cardsLength, uint32_t);
        Array cards = Array::New(env, cardsLength);
        for (uint32_t i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_SHUFFLE_DECK: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        break;
    }
    case MSG_SHUFFLE_HAND: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE(cardsLength, uint32_t);
        Array cards = Array::New(env, cardsLength);
        for (uint32_t i = 0; i < cardsLength; i++) {
            READ_VALUE(code, uint32_t);
            cards.Set(i, Number::New(env, code));
        }
        break;
    }
    case MSG_REFRESH_DECK: {
        break;
    }
    case MSG_SWAP_GRAVE_DECK: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(deck_size, uint32_t, Number, message);
        READ_VALUE(bufferLength, uint32_t);
        uint8_t* buffer = reader.read(bufferLength, eof);
        if (*eof) {
            return {};
        }
        Array returnedToExtra = Array::New(env);
        uint32_t returnedToExtraSize = 0;
        for (uint32_t i = 0; i < deck_size; i++) {
            uint32_t value = uint32_t(buffer[i / 8]) & (1 << (i % 8));
            if (value) {
                returnedToExtra.Set(returnedToExtraSize, i);
                returnedToExtraSize++;
            }
        }
        message.Set("returned_to_extra", returnedToExtra);
        break;
    }
    case MSG_SHUFFLE_SET_CARD: {
        READ_VALUE_SET(location, uint8_t, Number, message);
        READ_VALUE(cardsLength, uint32_t);

        Array cards = Array::New(env, cardsLength);
        for (uint32_t i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            Object valueFrom = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, valueFrom);
            READ_VALUE_SET(location, uint8_t, Number, valueFrom);
            READ_VALUE_SET(sequence, uint32_t, Number, valueFrom);
            READ_VALUE_SET(position, uint32_t, Number, valueFrom);
            value.Set("from", valueFrom);
            cards.Set(i, value);
        }
        for (uint32_t i = 0; i < cardsLength; i++) {
            Object valueTo = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, valueTo);
            READ_VALUE_SET(location, uint8_t, Number, valueTo);
            READ_VALUE_SET(sequence, uint32_t, Number, valueTo);
            READ_VALUE_SET(position, uint32_t, Number, valueTo);
            cards.Get(i).As<Object>().Set("to", valueTo);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_REVERSE_DECK: {
        break;
    }
    case MSG_DECK_TOP: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(count, uint32_t, Number, message);
        READ_VALUE_SET(code, uint32_t, Number, message);
        READ_VALUE_SET(position, uint32_t, Number, message);
        break;
    }
    case MSG_SHUFFLE_EXTRA: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE(cardsLength, uint32_t);
        Array cards = Array::New(env, cardsLength);
        for (uint32_t i = 0; i < cardsLength; i++) {
            READ_VALUE(code, uint32_t);
            cards.Set(i, Number::New(env, code));
        }
        break;
    }
    case MSG_NEW_TURN: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        break;
    }
    case MSG_NEW_PHASE: {
        READ_VALUE_SET(phase, uint16_t, Number, message);
        break;
    }
    case MSG_CONFIRM_EXTRATOP: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE(cardsLength, uint32_t);
        Array cards = Array::New(env, cardsLength);
        for (uint32_t i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_MOVE: {
        READ_VALUE_SET(code, uint32_t, Number, message);
        {
            Object valueFrom = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, valueFrom);
            READ_VALUE_SET(location, uint8_t, Number, valueFrom);
            READ_VALUE_SET(sequence, uint32_t, Number, valueFrom);
            READ_VALUE_SET(position, uint32_t, Number, valueFrom);
            message.Set("from", valueFrom);
        }
        {
            Object valueTo = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, valueTo);
            READ_VALUE_SET(location, uint8_t, Number, valueTo);
            READ_VALUE_SET(sequence, uint32_t, Number, valueTo);
            READ_VALUE_SET(position, uint32_t, Number, valueTo);
            message.Set("to", valueTo);
        }
        break;
    }
    case MSG_POS_CHANGE: {
        READ_VALUE_SET(code, uint32_t, Number, message);
        READ_VALUE_SET(controller, uint8_t, Number, message);
        READ_VALUE_SET(location, uint8_t, Number, message);
        READ_VALUE_SET(sequence, uint8_t, Number, message);
        READ_VALUE_SET(prev_position, uint8_t, Number, message);
        READ_VALUE_SET(position, uint8_t, Number, message);
        break;
    }
    case MSG_SET: {
        READ_VALUE_SET(code, uint32_t, Number, message);
        READ_VALUE_SET(controller, uint8_t, Number, message);
        READ_VALUE_SET(location, uint8_t, Number, message);
        READ_VALUE_SET(sequence, uint32_t, Number, message);
        READ_VALUE_SET(position, uint32_t, Number, message);
        break;
    }
    case MSG_SWAP: {
        {
            Object valueCard1 = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, valueCard1);
            READ_VALUE_SET(controller, uint8_t, Number, valueCard1);
            READ_VALUE_SET(location, uint8_t, Number, valueCard1);
            READ_VALUE_SET(sequence, uint32_t, Number, valueCard1);
            READ_VALUE_SET(position, uint32_t, Number, valueCard1);
            message.Set("card1", valueCard1);
        }
        {
            Object valueCard2 = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, valueCard2);
            READ_VALUE_SET(controller, uint8_t, Number, valueCard2);
            READ_VALUE_SET(location, uint8_t, Number, valueCard2);
            READ_VALUE_SET(sequence, uint32_t, Number, valueCard2);
            READ_VALUE_SET(position, uint32_t, Number, valueCard2);
            message.Set("card2", valueCard2);
        }
        break;
    }
    case MSG_FIELD_DISABLED: {
        READ_VALUE_SET(field_mask, uint32_t, Number, message);
        break;
    }
    case MSG_SUMMONING: {
        READ_VALUE_SET(code, uint32_t, Number, message);
        READ_VALUE_SET(controller, uint8_t, Number, message);
        READ_VALUE_SET(location, uint8_t, Number, message);
        READ_VALUE_SET(sequence, uint32_t, Number, message);
        READ_VALUE_SET(position, uint32_t, Number, message);
        break;
    }
    case MSG_SUMMONED: {
        break;
    }
    case MSG_SPSUMMONING: {
        READ_VALUE_SET(code, uint32_t, Number, message);
        READ_VALUE_SET(controller, uint8_t, Number, message);
        READ_VALUE_SET(location, uint8_t, Number, message);
        READ_VALUE_SET(sequence, uint32_t, Number, message);
        READ_VALUE_SET(position, uint32_t, Number, message);
        break;
    }
    case MSG_SPSUMMONED: {
        break;
    }
    case MSG_FLIPSUMMONING: {
        READ_VALUE_SET(code, uint32_t, Number, message);
        READ_VALUE_SET(controller, uint8_t, Number, message);
        READ_VALUE_SET(location, uint8_t, Number, message);
        READ_VALUE_SET(sequence, uint32_t, Number, message);
        READ_VALUE_SET(position, uint32_t, Number, message);
        break;
    }
    case MSG_FLIPSUMMONED: {
        break;
    }
    case MSG_CHAINING: {
        READ_VALUE_SET(code, uint32_t, Number, message);
        READ_VALUE_SET(controller, uint8_t, Number, message);
        READ_VALUE_SET(location, uint8_t, Number, message);
        READ_VALUE_SET(sequence, uint32_t, Number, message);
        READ_VALUE_SET(position, uint32_t, Number, message);
        READ_VALUE_SET(triggering_controller, uint8_t, Number, message);
        READ_VALUE_SET(triggering_location, uint8_t, Number, message);
        READ_VALUE_SET(triggering_sequence, uint32_t, Number, message);
        READ_VALUE_SET(description, uint64_t, BigInt, message);
        READ_VALUE_SET(chain_size, uint32_t, Number, message);
        break;
    }
    case MSG_CHAINED: {
        READ_VALUE_SET(chain_size, uint32_t, Number, message);
        break;
    }
    case MSG_CHAIN_SOLVING: {
        READ_VALUE_SET(chain_size, uint32_t, Number, message);
        break;
    }
    case MSG_CHAIN_SOLVED: {
        READ_VALUE_SET(chain_size, uint32_t, Number, message);
        break;
    }
    case MSG_CHAIN_END: {
        break;
    }
    case MSG_CHAIN_NEGATED: {
        READ_VALUE_SET(chain_size, uint32_t, Number, message);
        break;
    }
    case MSG_CHAIN_DISABLED: {
        READ_VALUE_SET(chain_size, uint32_t, Number, message);
        break;
    }
    case MSG_CARD_SELECTED: {
        READ_VALUE(cardsLength, uint32_t);
        Array cards = Array::New(env, cardsLength);
        for (uint32_t i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            READ_VALUE_SET(position, uint32_t, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_RANDOM_SELECTED: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE(cardsLength, uint32_t);
        Array cards = Array::New(env, cardsLength);
        for (uint32_t i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            READ_VALUE_SET(position, uint32_t, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_BECOME_TARGET: {
        READ_VALUE(cardsLength, uint32_t);
        Array cards = Array::New(env, cardsLength);
        for (uint32_t i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            READ_VALUE_SET(position, uint32_t, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_DRAW: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE(drawnLength, uint32_t);
        Array drawn = Array::New(env, drawnLength);
        for (uint32_t i = 0; i < drawnLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(position, uint32_t, Number, value);
            drawn.Set(i, value);
        }
        message.Set("drawn", drawn);
        break;
    }
    case MSG_DAMAGE: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(amount, uint32_t, Number, message);
        break;
    }
    case MSG_RECOVER: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(amount, uint32_t, Number, message);
        break;
    }
    case MSG_EQUIP: {
        {
            Object card = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, card);
            READ_VALUE_SET(location, uint8_t, Number, card);
            READ_VALUE_SET(sequence, uint32_t, Number, card);
            READ_VALUE_SET(position, uint32_t, Number, card);
            message.Set("card", card);
        }
        {
            Object target = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, target);
            READ_VALUE_SET(location, uint8_t, Number, target);
            READ_VALUE_SET(sequence, uint32_t, Number, target);
            READ_VALUE_SET(position, uint32_t, Number, target);
            message.Set("target", target);
        }
        break;
    }
    case MSG_LPUPDATE: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(lp, uint32_t, Number, message);
        break;
    }
    case MSG_UNEQUIP: {
        break;
    }
    case MSG_CARD_TARGET: {
        {
            Object card = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, card);
            READ_VALUE_SET(location, uint8_t, Number, card);
            READ_VALUE_SET(sequence, uint32_t, Number, card);
            READ_VALUE_SET(position, uint32_t, Number, card);
            message.Set("card", card);
        }
        {
            Object target = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, target);
            READ_VALUE_SET(location, uint8_t, Number, target);
            READ_VALUE_SET(sequence, uint32_t, Number, target);
            READ_VALUE_SET(position, uint32_t, Number, target);
            message.Set("target", target);
        }
        break;
    }
    case MSG_CANCEL_TARGET: {
        {
            Object card = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, card);
            READ_VALUE_SET(location, uint8_t, Number, card);
            READ_VALUE_SET(sequence, uint32_t, Number, card);
            READ_VALUE_SET(position, uint32_t, Number, card);
            message.Set("card", card);
        }
        {
            Object target = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, target);
            READ_VALUE_SET(location, uint8_t, Number, target);
            READ_VALUE_SET(sequence, uint32_t, Number, target);
            READ_VALUE_SET(position, uint32_t, Number, target);
            message.Set("target", target);
        }
        break;
    }
    case MSG_PAY_LPCOST: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(amount, uint32_t, Number, message);
        break;
    }
    case MSG_ADD_COUNTER: {
        READ_VALUE_SET(counter_type, uint16_t, Number, message);
        READ_VALUE_SET(controller, uint8_t, Number, message);
        READ_VALUE_SET(location, uint8_t, Number, message);
        READ_VALUE_SET(sequence, uint8_t, Number, message);
        READ_VALUE_SET(count, uint16_t, Number, message);
        break;
    }
    case MSG_REMOVE_COUNTER: {
        READ_VALUE_SET(counter_type, uint16_t, Number, message);
        READ_VALUE_SET(controller, uint8_t, Number, message);
        READ_VALUE_SET(location, uint8_t, Number, message);
        READ_VALUE_SET(sequence, uint8_t, Number, message);
        READ_VALUE_SET(count, uint16_t, Number, message);
        break;
    }
    case MSG_ATTACK: {
        {
            Object card = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, card);
            READ_VALUE_SET(location, uint8_t, Number, card);
            READ_VALUE_SET(sequence, uint32_t, Number, card);
            READ_VALUE_SET(position, uint32_t, Number, card);
            message.Set("card", card);
        }
        {
            READ_VALUE(controller, uint8_t);
            READ_VALUE(location, uint8_t);
            READ_VALUE(sequence, uint32_t);
            READ_VALUE(position, uint32_t);
            if (location == 0) {
                message.Set("target", env.Null());
            } else {
                Object target = Object::New(env);
                target.Set("controller", Number::New(env, controller));
                target.Set("location", Number::New(env, location));
                target.Set("sequence", Number::New(env, sequence));
                target.Set("position", Number::New(env, position));
                message.Set("target", target);
            }
            break;
        }
    }
    case MSG_BATTLE: {
        {
            Object card = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, card);
            READ_VALUE_SET(location, uint8_t, Number, card);
            READ_VALUE_SET(sequence, uint32_t, Number, card);
            READ_VALUE_SET(position, uint32_t, Number, card);
            READ_VALUE_SET(attack, uint32_t, Number, card);
            READ_VALUE_SET(defense, uint32_t, Number, card);
            READ_VALUE_SET(destroyed, uint8_t, Boolean, card);
            message.Set("card", card);
        }
        {
            READ_VALUE(controller, uint8_t);
            READ_VALUE(location, uint8_t);
            READ_VALUE(sequence, uint32_t);
            READ_VALUE(position, uint32_t);
            READ_VALUE(attack, uint32_t);
            READ_VALUE(defense, uint32_t);
            READ_VALUE(destroyed, uint8_t);
            if (location == 0) {
                message.Set("target", env.Null());
            } else {
                Object target = Object::New(env);
                target.Set("controller", Number::New(env, controller));
                target.Set("location", Number::New(env, location));
                target.Set("sequence", Number::New(env, sequence));
                target.Set("position", Number::New(env, position));
                target.Set("attack", Number::New(env, attack));
                target.Set("defense", Number::New(env, defense));
                target.Set("destroyed", Boolean::New(env, destroyed));
                message.Set("target", target);
            }
        }
        break;
    }
    case MSG_ATTACK_DISABLED: {
        break;
    }
    case MSG_DAMAGE_STEP_START: {
        break;
    }
    case MSG_DAMAGE_STEP_END: {
        break;
    }
    case MSG_MISSED_EFFECT: {
        READ_VALUE_SET(controller, uint8_t, Number, message);
        READ_VALUE_SET(location, uint8_t, Number, message);
        READ_VALUE_SET(sequence, uint32_t, Number, message);
        READ_VALUE_SET(position, uint32_t, Number, message);
        READ_VALUE_SET(code, uint32_t, Number, message);
        break;
    }
    case MSG_BE_CHAIN_TARGET: {
        break;
    }
    case MSG_CREATE_RELATION: {
        break;
    }
    case MSG_RELEASE_RELATION: {
        break;
    }
    case MSG_TOSS_COIN: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE(resultsLength, uint8_t);
        Array results = Array::New(env, resultsLength);
        for (uint8_t i = 0; i < resultsLength; i++) {
            READ_VALUE(result, uint8_t);
            results.Set(i, Boolean::New(env, result));
        }
        message.Set("results", results);
        break;
    }
    case MSG_TOSS_DICE: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE(resultsLength, uint8_t);
        Array results = Array::New(env, resultsLength);
        for (uint8_t i = 0; i < resultsLength; i++) {
            READ_VALUE(result, uint8_t);
            results.Set(uint32_t(i), Number::New(env, result));
        }
        message.Set("results", results);
        break;
    }
    case MSG_ROCK_PAPER_SCISSORS: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        break;
    }
    case MSG_HAND_RES: {
        READ_VALUE(result, uint8_t);
        Array results = Array::New(env, 2);
        results.Set(uint32_t(0), Number::New(env, result & 0b11));
        results.Set(uint32_t(1), Number::New(env, (result >> 2) & 0b11));
        message.Set("results", results);
        break;
    }
    case MSG_ANNOUNCE_RACE: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(count, uint8_t, Number, message);
        READ_VALUE_SET(available, uint8_t, Number, message);
        break;
    }
    case MSG_ANNOUNCE_ATTRIB: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(count, uint8_t, Number, message);
        READ_VALUE_SET(available, uint8_t, Number, message);
        break;
    }
    case MSG_ANNOUNCE_CARD: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE(opcodesLength, uint8_t);
        Array opcodes = Array::New(env, opcodesLength);
        for (uint8_t i = 0; i < opcodesLength; i++) {
            READ_VALUE(opcode, uint64_t);
            opcodes.Set(uint32_t(i), BigInt::New(env, opcode));
        }
        message.Set("opcodes", opcodes);
        break;
    }
    case MSG_ANNOUNCE_NUMBER: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE(optionsLength, uint8_t);
        Array options = Array::New(env, optionsLength);
        for (uint8_t i = 0; i < optionsLength; i++) {
            READ_VALUE(option, uint64_t);
            options.Set(uint32_t(i), Number::New(env, option));
        }
        message.Set("options", options);
        break;
    }
    case MSG_CARD_HINT: {
        READ_VALUE_SET(controller, uint8_t, Number, message);
        READ_VALUE_SET(location, uint8_t, Number, message);
        READ_VALUE_SET(sequence, uint32_t, Number, message);
        READ_VALUE_SET(position, uint32_t, Number, message);
        READ_VALUE_SET(card_hint, uint8_t, Number, message);
        READ_VALUE_SET(description, uint64_t, BigInt, message);
        break;
    }
    case MSG_TAG_SWAP: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(deck_size, uint32_t, Number, message);
        READ_VALUE(extraLength, uint32_t);
        READ_VALUE_SET(extra_faceup_count, uint32_t, Number, message);
        READ_VALUE(handLength, uint32_t);
        READ_VALUE_VSET(deck_top_card, uint32_t,
                        deck_top_card == 0 ? env.Null()
                                           : Number::New(env, deck_top_card),
                        message);

        Array hand = Array::New(env, handLength);
        for (uint32_t i = 0; i < handLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(position, uint32_t, Number, value);
            hand.Set(i, value);
        }
        message.Set("hand", hand);

        Array extra = Array::New(env, extraLength);
        for (uint32_t i = 0; i < extraLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(position, uint32_t, Number, value);
            extra.Set(i, value);
        }
        message.Set("extra", extra);
        break;
    }
    case MSG_RELOAD_FIELD: {
        READ_VALUE_SET(flags, uint32_t, Number, message);
        Array players = Array::New(env, 2);
        for (uint32_t i = 0; i < 2; i++) {
            Object player = Object::New(env);
            READ_VALUE_SET(lp, uint32_t, Number, player);
            Array monsters = Array::New(env, 7);
            for (uint32_t mi = 0; mi < 7; mi++) {
                Object card = Object::New(env);
                READ_VALUE(hasCard, uint8_t);
                if (hasCard == 1) {
                    READ_VALUE_SET(position, uint8_t, Number, card);
                    READ_VALUE_SET(materials, uint32_t, Number, card);
                }
                monsters.Set(mi, card);
            }
            player.Set("monsters", monsters);
            Array spells = Array::New(env, 8);
            for (uint32_t si = 0; si < 8; si++) {
                Object card = Object::New(env);
                READ_VALUE(hasCard, uint8_t);
                if (hasCard == 1) {
                    READ_VALUE_SET(position, uint8_t, Number, card);
                    READ_VALUE_SET(materials, uint32_t, Number, card);
                }
                monsters.Set(si, card);
            }
            player.Set("spells", spells);
            READ_VALUE_SET(deck_size, uint32_t, Number, player);
            READ_VALUE_SET(hand_size, uint32_t, Number, player);
            READ_VALUE_SET(grave_size, uint32_t, Number, player);
            READ_VALUE_SET(banish_size, uint32_t, Number, player);
            READ_VALUE_SET(extra_size, uint32_t, Number, player);
            READ_VALUE_SET(extra_faceup_count, uint32_t, Number, player);

            players.Set(i, player);
        }
        READ_VALUE(chainLength, uint32_t);
        Array chain = Array::New(env, chainLength);
        for (uint32_t i = 0; i < chainLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32_t, Number, value);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            READ_VALUE_SET(position, uint32_t, Number, value);
            READ_VALUE_SET(triggering_controller, uint8_t, Number, value);
            READ_VALUE_SET(triggering_location, uint8_t, Number, value);
            READ_VALUE_SET(triggering_sequence, uint32_t, Number, value);
            READ_VALUE_SET(description, uint64_t, BigInt, value);
            chain.Set(i, value);
        }
        message.Set("chain", chain);
        break;
    }
    case MSG_AI_NAME: {
        // debug stuff
        break;
    }
    case MSG_SHOW_HINT: {
        // debug stuff
        break;
    }
    case MSG_PLAYER_HINT: {
        READ_VALUE_SET(player, uint8_t, Number, message);
        READ_VALUE_SET(player_hint, uint8_t, Number, message);
        READ_VALUE_SET(description, uint64_t, BigInt, message);
        break;
    }
    case MSG_MATCH_KILL: {
        READ_VALUE_SET(card, uint32_t, Number, message);
        break;
    }
    case MSG_CUSTOM_MSG: {
        break;
    }
    case MSG_REMOVE_CARDS: {
        READ_VALUE(cardsLength, uint32_t);
        Array cards = Array::New(env, cardsLength);
        for (uint32_t i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(controller, uint8_t, Number, value);
            READ_VALUE_SET(location, uint8_t, Number, value);
            READ_VALUE_SET(sequence, uint32_t, Number, value);
            READ_VALUE_SET(position, uint32_t, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    }
    return message;
}

NODE_API_MODULE(addon, Init)
