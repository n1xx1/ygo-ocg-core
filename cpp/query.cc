#include "query.h"
#include "common_macros.h"
#include <unordered_map>

#define READ_VALUE(name, type) IMPL_READ_VALUE("query", false, *eof, name, type)

#define READ_VALUE_SET(name, type, jstype, jstarget)                           \
    IMPL_READ_VALUE_SET("query", false, *eof, name, type, jstype, jstarget)

#define READ_VALUE_VSET(name, type, jstype, jstarget)                          \
    IMPL_READ_VALUE_VSET("query", false, *eof, name, type, jstype, jstarget)

#define CASE_BASIC_VALUE(query, name, type, jsvalue, jstarget)                 \
    case query: {                                                              \
        READ_VALUE_SET(name, type, jsvalue, jstarget);                         \
        break;                                                                 \
    }

std::unordered_map<uint32, const char*> debugQueryType = {
    {QUERY_CODE, "QUERY_CODE"},
    {QUERY_POSITION, "QUERY_POSITION"},
    {QUERY_ALIAS, "QUERY_ALIAS"},
    {QUERY_TYPE, "QUERY_TYPE"},
    {QUERY_LEVEL, "QUERY_LEVEL"},
    {QUERY_RANK, "QUERY_RANK"},
    {QUERY_ATTRIBUTE, "QUERY_ATTRIBUTE"},
    {QUERY_RACE, "QUERY_RACE"},
    {QUERY_ATTACK, "QUERY_ATTACK"},
    {QUERY_DEFENSE, "QUERY_DEFENSE"},
    {QUERY_BASE_ATTACK, "QUERY_BASE_ATTACK"},
    {QUERY_BASE_DEFENSE, "QUERY_BASE_DEFENSE"},
    {QUERY_REASON, "QUERY_REASON"},
    {QUERY_REASON_CARD, "QUERY_REASON_CARD"},
    {QUERY_EQUIP_CARD, "QUERY_EQUIP_CARD"},
    {QUERY_TARGET_CARD, "QUERY_TARGET_CARD"},
    {QUERY_OVERLAY_CARD, "QUERY_OVERLAY_CARD"},
    {QUERY_COUNTERS, "QUERY_COUNTERS"},
    {QUERY_OWNER, "QUERY_OWNER"},
    {QUERY_STATUS, "QUERY_STATUS"},
    {QUERY_IS_PUBLIC, "QUERY_IS_PUBLIC"},
    {QUERY_LSCALE, "QUERY_LSCALE"},
    {QUERY_RSCALE, "QUERY_RSCALE"},
    {QUERY_LINK, "QUERY_LINK"},
    {QUERY_IS_HIDDEN, "QUERY_IS_HIDDEN"},
    {QUERY_COVER, "QUERY_COVER"},
    {QUERY_END, "QUERY_END"},
};

bool parseQuery(Env& env, BufferReader& reader, Object& value, bool* eof) {
    READ_VALUE(queryType, uint32);

    // printf("queryType: %s\n", debugQueryType[queryType]);

    switch (queryType) {
        CASE_BASIC_VALUE(QUERY_CODE, code, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_POSITION, position, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_ALIAS, alias, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_TYPE, type, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_LEVEL, level, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_RANK, rank, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_ATTRIBUTE, attribute, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_RACE, race, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_ATTACK, attack, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_DEFENSE, defense, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_BASE_ATTACK, base_attack, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_BASE_DEFENSE, base_defence, uint32, Number,
                         value);
        CASE_BASIC_VALUE(QUERY_REASON, reason, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_COVER, cover, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_OWNER, owner, uint8, Number, value);
        CASE_BASIC_VALUE(QUERY_STATUS, status, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_IS_PUBLIC, is_public, uint8, Boolean, value);
        CASE_BASIC_VALUE(QUERY_IS_HIDDEN, is_hidden, uint8, Boolean, value);
        CASE_BASIC_VALUE(QUERY_LSCALE, lscale, uint32, Number, value);
        CASE_BASIC_VALUE(QUERY_RSCALE, rscale, uint32, Number, value);

    case QUERY_REASON_CARD: {
        Object reason_card = Object::New(env);
        READ_VALUE_SET(controller, uint8, Number, reason_card);
        READ_VALUE_SET(location, uint8, Number, reason_card);
        READ_VALUE_SET(sequence, uint32, Number, reason_card);
        READ_VALUE_SET(position, uint32, Number, reason_card);
        if (controller == 0 && location == 0 && sequence == 0 &&
            position == 0) {
            value.Set("reason_card", env.Null());
        } else {
            value.Set("reason_card", reason_card);
        }
        break;
    }
    case QUERY_EQUIP_CARD: {
        Object equip_card = Object::New(env);
        READ_VALUE_SET(controller, uint8, Number, equip_card);
        READ_VALUE_SET(location, uint8, Number, equip_card);
        READ_VALUE_SET(sequence, uint32, Number, equip_card);
        READ_VALUE_SET(position, uint32, Number, equip_card);
        if (controller == 0 && location == 0 && sequence == 0 &&
            position == 0) {
            value.Set("equip_card", env.Null());
        } else {
            value.Set("equip_card", equip_card);
        }
        break;
    }
    case QUERY_TARGET_CARD: {
        READ_VALUE(targetsLength, uint32);
        Array targets = Array::New(env, targetsLength);
        for (uint32 i = 0; i < targetsLength; i++) {
            Object target = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, target);
            READ_VALUE_SET(location, uint8, Number, target);
            READ_VALUE_SET(sequence, uint32, Number, target);
            READ_VALUE_SET(position, uint32, Number, target);
            targets.Set(i, target);
        }
        value.Set("target_cards", targets);
        break;
    }
    case QUERY_OVERLAY_CARD: {
        READ_VALUE(overlaysLength, uint32);
        Array overlays = Array::New(env, overlaysLength);
        for (uint32 i = 0; i < overlaysLength; i++) {
            READ_VALUE(code, uint32);
            overlays.Set(i, Number::New(env, code));
        }
        value.Set("overlay_cards", overlays);
        break;
    }
    case QUERY_COUNTERS: {
        READ_VALUE(countersLength, uint32);
        Array counters = Array::New(env, countersLength);
        for (uint32 i = 0; i < countersLength; i++) {
            READ_VALUE(counterValue, uint32);
            Object counter = Object::New(env);
            counter.Set("counter", Number::New(env, counterValue & 0xffff));
            counter.Set("count", Number::New(env, counterValue >> 16));
            counters.Set(i, counter);
        }
        value.Set("counters", counters);
        break;
    }
    case QUERY_LINK: {
        Object linkValue = Object::New(env);
        READ_VALUE_SET(link, uint32, Number, linkValue);
        READ_VALUE_SET(markers, uint32, Number, linkValue);
        value.Set("link", linkValue);
        break;
    }
    case QUERY_END: {
        return true;
    }
    }
    return false;
}

#undef READ_VALUE
#undef READ_VALUE_SET
#undef READ_VALUE_VSET
#undef CASE_BASIC_VALUE

#define READ_VALUE(name, type)                                                 \
    IMPL_READ_VALUE("field", env.Undefined(), *eof, name, type)

#define READ_VALUE_SET(name, type, jstype, jstarget)                           \
    IMPL_READ_VALUE_SET("field", env.Undefined(), *eof, name, type, jstype,    \
                        jstarget)

#define READ_VALUE_VSET(name, type, jstype, jstarget)                          \
    IMPL_READ_VALUE_VSET("field", env.Undefined(), *eof, name, type, jstype,   \
                         jstarget)

Value parseField(Env& env, BufferReader& reader, bool* eof) {
    Object value = Object::New(env);

    READ_VALUE_SET(flags, uint32, Number, value);
    Array players = Array::New(env, 2);
    for (uint32 i = 0; i < 2; i++) {
        Object player = Object::New(env);
        READ_VALUE_SET(lp, int32, Number, player);
        Array monsters = Array::New(env, 7);
        for (uint32 mi = 0; mi < 7; mi++) {
            READ_VALUE(hasCard, uint8);
            if (hasCard == 1) {
                Object card = Object::New(env);
                READ_VALUE_SET(position, uint8, Number, card);
                READ_VALUE_SET(materials, uint32, Number, card);
                monsters.Set(mi, card);
            } else {
                monsters.Set(mi, env.Null());
            }
        }
        player.Set("monsters", monsters);
        Array spells = Array::New(env, 8);
        for (uint32 si = 0; si < 8; si++) {
            READ_VALUE(hasCard, uint8);
            if (hasCard == 1) {
                Object card = Object::New(env);
                READ_VALUE_SET(position, uint8, Number, card);
                READ_VALUE_SET(materials, uint32, Number, card);
                spells.Set(si, card);
            } else {
                spells.Set(si, env.Null());
            }
        }
        player.Set("spells", spells);
        READ_VALUE_SET(deck_size, uint32, Number, player);
        READ_VALUE_SET(hand_size, uint32, Number, player);
        READ_VALUE_SET(grave_size, uint32, Number, player);
        READ_VALUE_SET(banish_size, uint32, Number, player);
        READ_VALUE_SET(extra_size, uint32, Number, player);
        READ_VALUE_SET(extra_faceup_count, uint32, Number, player);

        players.Set(i, player);
    }
    value.Set("players", players);

    READ_VALUE(chainLength, uint32);
    Array chain = Array::New(env, chainLength);
    for (uint32 i = 0; i < chainLength; i++) {
        Object value = Object::New(env);
        READ_VALUE_SET(code, int32, Number, value);
        READ_VALUE_SET(controller, uint8, Number, value);
        READ_VALUE_SET(location, uint8, Number, value);
        READ_VALUE_SET(sequence, uint32, Number, value);
        READ_VALUE_SET(position, uint32, Number, value);
        READ_VALUE_SET(triggering_controller, uint8, Number, value);
        READ_VALUE_SET(triggering_location, uint8, Number, value);
        READ_VALUE_SET(triggering_sequence, uint32, Number, value);
        READ_VALUE_SET(description, uint64, BigInt, value);
        chain.Set(i, value);
    }
    value.Set("chain", chain);
    return value;
}
