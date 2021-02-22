#include "messages.h"

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
    READ_VALUE(messageType, uint8);

    Object message = Object::New(env);
    message.Set("type", Number::New(env, messageType));

    switch (messageType) {
    case MSG_HINT: {
        READ_VALUE_SET(hint_type, uint8, Number, message);
        READ_VALUE_SET(player, uint8, Number, message);
        if (reader.avail() > 4) {
            READ_VALUE_SET(hint, uint64, BigInt, message);
        } else {
            READ_VALUE_VSET(hint, uint32, BigInt::New(env, uint64(hint)),
                            message);
        }
        break;
    }
    case MSG_WIN: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(reason, uint8, Number, message);
        break;
    }
    case MSG_SELECT_BATTLECMD: {
        READ_VALUE_SET(player, uint8, Number, message);

        READ_VALUE(chainLength, uint32);
        Array chains = Array::New(env, chainLength);
        for (uint32 i = 0; i < chainLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            READ_VALUE_SET(description, uint64, Number, value);
            READ_VALUE_SET(client_mode, uint8, Number, value);
            chains.Set(i, value);
        }
        message.Set("chains", chains);

        READ_VALUE(attackLength, uint32);
        Array attacks = Array::New(env, attackLength);
        for (uint32 i = 0; i < attackLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint8, Number, value);
            READ_VALUE_SET(can_direct, uint8, Boolean, value);
            attacks.Set(i, value);
        }
        message.Set("attacks", attacks);

        READ_VALUE_SET(to_m2, uint8, Boolean, message);
        READ_VALUE_SET(to_ep, uint8, Boolean, message);
        break;
    }
    case MSG_SELECT_IDLECMD: {
        READ_VALUE_SET(player, uint8, Number, message);

        READ_VALUE(summonLength, uint32);
        Array summons = Array::New(env, summonLength);
        for (uint32 i = 0; i < summonLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            summons.Set(i, value);
        }
        message.Set("summons", summons);

        READ_VALUE(spSummonLength, uint32);
        Array spSummons = Array::New(env, spSummonLength);
        for (uint32 i = 0; i < spSummonLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            spSummons.Set(i, value);
        }
        message.Set("sp_summons", spSummons);

        READ_VALUE(posChangeLength, uint32);
        Array posChanges = Array::New(env, posChangeLength);
        for (uint32 i = 0; i < posChangeLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint8, Number, value);
            posChanges.Set(i, value);
        }
        message.Set("pos_changes", posChanges);

        READ_VALUE(monsterSetLength, uint32);
        Array monsterSets = Array::New(env, monsterSetLength);
        for (uint32 i = 0; i < monsterSetLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            monsterSets.Set(i, value);
        }
        message.Set("monster_sets", monsterSets);

        READ_VALUE(spellSetLength, uint32);
        Array spellSets = Array::New(env, spellSetLength);
        for (uint32 i = 0; i < spellSetLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            spellSets.Set(i, value);
        }
        message.Set("spell_sets", spellSets);

        READ_VALUE(activateLength, uint32);
        Array activates = Array::New(env, activateLength);
        for (uint32 i = 0; i < activateLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            READ_VALUE_SET(description, uint64, BigInt, value);
            READ_VALUE_SET(client_mode, uint8, Number, value);
            activates.Set(i, value);
        }
        message.Set("activates", activates);

        READ_VALUE_SET(to_bp, uint8, Boolean, message);
        READ_VALUE_SET(to_ep, uint8, Boolean, message);
        READ_VALUE_SET(shuffle, uint8, Boolean, message);
        break;
    }
    case MSG_SELECT_EFFECTYN: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(code, uint32, Number, message);
        READ_VALUE_SET(controller, uint8, Number, message);
        READ_VALUE_SET(location, uint8, Number, message);
        READ_VALUE_SET(sequence, uint32, Number, message);
        READ_VALUE_SET(position, uint32, Number, message);
        READ_VALUE_SET(description, uint64, BigInt, message);
        break;
    }
    case MSG_SELECT_YESNO: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(description, uint64, BigInt, message);
        break;
    }
    case MSG_SELECT_OPTION: {
        READ_VALUE_SET(player, uint8, Number, message);

        READ_VALUE(optionsLength, uint8);
        Array options = Array::New(env, optionsLength);
        for (uint8 i = 0; i < optionsLength; i++) {
            READ_VALUE(option, uint64);
            options.Set(uint32(i), BigInt::New(env, option));
        }
        message.Set("options", options);
        break;
    }
    case MSG_SELECT_CARD: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(can_cancel, uint8, Boolean, message);
        READ_VALUE_SET(min, uint32, Number, message);
        READ_VALUE_SET(max, uint32, Number, message);

        READ_VALUE(selectsLength, uint32);
        Array selects = Array::New(env, selectsLength);
        for (uint32 i = 0; i < selectsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            READ_VALUE_SET(position, uint32, Number, value);
            selects.Set(i, value);
        }
        message.Set("selects", selects);

        break;
    }
    case MSG_SELECT_CHAIN: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(spe_count, uint8, Number, message);
        READ_VALUE_SET(forced, uint8, Boolean, message);
        READ_VALUE_SET(hint_timing, uint32, Number, message);
        READ_VALUE_SET(hint_timing_other, uint32, Number, message);

        READ_VALUE(selectsLength, uint32);
        Array selects = Array::New(env, selectsLength);
        for (uint32 i = 0; i < selectsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            READ_VALUE_SET(position, uint32, Number, value);
            READ_VALUE_SET(description, uint64, BigInt, value);
            READ_VALUE_SET(client_mode, uint8, Number, value);
            selects.Set(i, value);
        }
        message.Set("selects", selects);
        break;
    }
    case MSG_SELECT_PLACE: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(count, uint8, Number, message);
        READ_VALUE_SET(field_mask, uint32, Number, message);
        break;
    }
    case MSG_SELECT_POSITION: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(code, uint32, Number, message);
        READ_VALUE_SET(positions, uint8, Number, message);
        break;
    }
    case MSG_SELECT_TRIBUTE: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(can_cancel, uint8, Boolean, message);
        READ_VALUE_SET(min, uint32, Number, message);
        READ_VALUE_SET(max, uint32, Number, message);

        READ_VALUE(selectsLength, uint32);
        Array selects = Array::New(env, selectsLength);
        for (uint32 i = 0; i < selectsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            READ_VALUE_SET(release_param, uint8, Number, value);
            selects.Set(i, value);
        }
        message.Set("selects", selects);
        break;
    }
    case MSG_SORT_CHAIN: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE(cardsLength, uint32);
        Array cards = Array::New(env, cardsLength);
        for (uint32 i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint32, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_SELECT_COUNTER: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(counter_type, uint16, Number, message);
        READ_VALUE_SET(count, uint16, Number, message);
        READ_VALUE(cardsLength, uint32);
        Array cards = Array::New(env, cardsLength);
        for (uint32 i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint8, Number, value);
            READ_VALUE_SET(count, uint16, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_SELECT_SUM: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(select_max, uint8, Boolean, message);
        READ_VALUE_SET(amount, uint32, Number, message);
        READ_VALUE_SET(min, uint32, Number, message);
        READ_VALUE_SET(max, uint32, Number, message);
        READ_VALUE(selectsMustLength, uint32);
        Array selectsMust = Array::New(env, selectsMustLength);
        for (uint32 i = 0; i < selectsMustLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            READ_VALUE_SET(amount, uint32, Number, value);
            selectsMust.Set(i, value);
        }
        message.Set("selects_must", selectsMust);

        READ_VALUE(selectLength, uint32);
        Array select = Array::New(env, selectLength);
        for (uint32 i = 0; i < selectLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            READ_VALUE_SET(amount, uint32, Number, value);
            select.Set(i, value);
        }
        message.Set("selects", select);
        break;
    }
    case MSG_SELECT_DISFIELD: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(count, uint8, Number, message);
        READ_VALUE_SET(field_mask, uint32, Number, message);
        break;
    }
    case MSG_SORT_CARD: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE(cardsLength, uint32);
        Array cards = Array::New(env, cardsLength);
        for (uint32 i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint32, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_SELECT_UNSELECT_CARD: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(can_finish, uint8, Boolean, message);
        READ_VALUE_SET(can_cancel, uint8, Boolean, message);
        READ_VALUE_SET(min, uint32, Boolean, message);
        READ_VALUE_SET(max, uint32, Boolean, message);

        READ_VALUE(selectCardsLength, uint32);
        Array selectCards = Array::New(env, selectCardsLength);
        for (uint32 i = 0; i < selectCardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            READ_VALUE_SET(position, uint32, Number, value);
            selectCards.Set(i, value);
        }
        message.Set("select_cards", selectCards);

        READ_VALUE(unselectCardsLength, uint32);
        Array unselectCards = Array::New(env, unselectCardsLength);
        for (uint32 i = 0; i < unselectCardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            READ_VALUE_SET(position, uint32, Number, value);
            unselectCards.Set(i, value);
        }
        message.Set("unselect_cards", unselectCards);

        break;
    }
    case MSG_CONFIRM_DECKTOP: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE(cardsLength, uint32);
        Array cards = Array::New(env, cardsLength);
        for (uint32 i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_CONFIRM_CARDS: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE(cardsLength, uint32);
        Array cards = Array::New(env, cardsLength);
        for (uint32 i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_SHUFFLE_DECK: {
        READ_VALUE_SET(player, uint8, Number, message);
        break;
    }
    case MSG_SHUFFLE_HAND: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE(cardsLength, uint32);
        Array cards = Array::New(env, cardsLength);
        for (uint32 i = 0; i < cardsLength; i++) {
            READ_VALUE(code, uint32);
            cards.Set(i, Number::New(env, code));
        }
        break;
    }
    case MSG_REFRESH_DECK: {
        break;
    }
    case MSG_SWAP_GRAVE_DECK: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(deck_size, uint32, Number, message);
        READ_VALUE(bufferLength, uint32);
        uint8* buffer = reader.read(bufferLength, eof);
        if (*eof) {
            return {};
        }
        Array returnedToExtra = Array::New(env);
        uint32 returnedToExtraSize = 0;
        for (uint32 i = 0; i < deck_size; i++) {
            uint32 value = uint32(buffer[i / 8]) & (1 << (i % 8));
            if (value) {
                returnedToExtra.Set(returnedToExtraSize, i);
                returnedToExtraSize++;
            }
        }
        message.Set("returned_to_extra", returnedToExtra);
        break;
    }
    case MSG_SHUFFLE_SET_CARD: {
        READ_VALUE_SET(location, uint8, Number, message);
        READ_VALUE(cardsLength, uint32);

        Array cards = Array::New(env, cardsLength);
        for (uint32 i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            Object valueFrom = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, valueFrom);
            READ_VALUE_SET(location, uint8, Number, valueFrom);
            READ_VALUE_SET(sequence, uint32, Number, valueFrom);
            READ_VALUE_SET(position, uint32, Number, valueFrom);
            value.Set("from", valueFrom);
            cards.Set(i, value);
        }
        for (uint32 i = 0; i < cardsLength; i++) {
            Object valueTo = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, valueTo);
            READ_VALUE_SET(location, uint8, Number, valueTo);
            READ_VALUE_SET(sequence, uint32, Number, valueTo);
            READ_VALUE_SET(position, uint32, Number, valueTo);
            cards.Get(i).As<Object>().Set("to", valueTo);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_REVERSE_DECK: {
        break;
    }
    case MSG_DECK_TOP: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(count, uint32, Number, message);
        READ_VALUE_SET(code, uint32, Number, message);
        READ_VALUE_SET(position, uint32, Number, message);
        break;
    }
    case MSG_SHUFFLE_EXTRA: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE(cardsLength, uint32);
        Array cards = Array::New(env, cardsLength);
        for (uint32 i = 0; i < cardsLength; i++) {
            READ_VALUE(code, uint32);
            cards.Set(i, Number::New(env, code));
        }
        break;
    }
    case MSG_NEW_TURN: {
        READ_VALUE_SET(player, uint8, Number, message);
        break;
    }
    case MSG_NEW_PHASE: {
        READ_VALUE_SET(phase, uint16, Number, message);
        break;
    }
    case MSG_CONFIRM_EXTRATOP: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE(cardsLength, uint32);
        Array cards = Array::New(env, cardsLength);
        for (uint32 i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_MOVE: {
        READ_VALUE_SET(code, uint32, Number, message);
        {
            Object valueFrom = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, valueFrom);
            READ_VALUE_SET(location, uint8, Number, valueFrom);
            READ_VALUE_SET(sequence, uint32, Number, valueFrom);
            READ_VALUE_SET(position, uint32, Number, valueFrom);
            message.Set("from", valueFrom);
        }
        {
            Object valueTo = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, valueTo);
            READ_VALUE_SET(location, uint8, Number, valueTo);
            READ_VALUE_SET(sequence, uint32, Number, valueTo);
            READ_VALUE_SET(position, uint32, Number, valueTo);
            message.Set("to", valueTo);
        }
        break;
    }
    case MSG_POS_CHANGE: {
        READ_VALUE_SET(code, uint32, Number, message);
        READ_VALUE_SET(controller, uint8, Number, message);
        READ_VALUE_SET(location, uint8, Number, message);
        READ_VALUE_SET(sequence, uint8, Number, message);
        READ_VALUE_SET(prev_position, uint8, Number, message);
        READ_VALUE_SET(position, uint8, Number, message);
        break;
    }
    case MSG_SET: {
        READ_VALUE_SET(code, uint32, Number, message);
        READ_VALUE_SET(controller, uint8, Number, message);
        READ_VALUE_SET(location, uint8, Number, message);
        READ_VALUE_SET(sequence, uint32, Number, message);
        READ_VALUE_SET(position, uint32, Number, message);
        break;
    }
    case MSG_SWAP: {
        {
            Object valueCard1 = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, valueCard1);
            READ_VALUE_SET(controller, uint8, Number, valueCard1);
            READ_VALUE_SET(location, uint8, Number, valueCard1);
            READ_VALUE_SET(sequence, uint32, Number, valueCard1);
            READ_VALUE_SET(position, uint32, Number, valueCard1);
            message.Set("card1", valueCard1);
        }
        {
            Object valueCard2 = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, valueCard2);
            READ_VALUE_SET(controller, uint8, Number, valueCard2);
            READ_VALUE_SET(location, uint8, Number, valueCard2);
            READ_VALUE_SET(sequence, uint32, Number, valueCard2);
            READ_VALUE_SET(position, uint32, Number, valueCard2);
            message.Set("card2", valueCard2);
        }
        break;
    }
    case MSG_FIELD_DISABLED: {
        READ_VALUE_SET(field_mask, uint32, Number, message);
        break;
    }
    case MSG_SUMMONING: {
        READ_VALUE_SET(code, uint32, Number, message);
        READ_VALUE_SET(controller, uint8, Number, message);
        READ_VALUE_SET(location, uint8, Number, message);
        READ_VALUE_SET(sequence, uint32, Number, message);
        READ_VALUE_SET(position, uint32, Number, message);
        break;
    }
    case MSG_SUMMONED: {
        break;
    }
    case MSG_SPSUMMONING: {
        READ_VALUE_SET(code, uint32, Number, message);
        READ_VALUE_SET(controller, uint8, Number, message);
        READ_VALUE_SET(location, uint8, Number, message);
        READ_VALUE_SET(sequence, uint32, Number, message);
        READ_VALUE_SET(position, uint32, Number, message);
        break;
    }
    case MSG_SPSUMMONED: {
        break;
    }
    case MSG_FLIPSUMMONING: {
        READ_VALUE_SET(code, uint32, Number, message);
        READ_VALUE_SET(controller, uint8, Number, message);
        READ_VALUE_SET(location, uint8, Number, message);
        READ_VALUE_SET(sequence, uint32, Number, message);
        READ_VALUE_SET(position, uint32, Number, message);
        break;
    }
    case MSG_FLIPSUMMONED: {
        break;
    }
    case MSG_CHAINING: {
        READ_VALUE_SET(code, uint32, Number, message);
        READ_VALUE_SET(controller, uint8, Number, message);
        READ_VALUE_SET(location, uint8, Number, message);
        READ_VALUE_SET(sequence, uint32, Number, message);
        READ_VALUE_SET(position, uint32, Number, message);
        READ_VALUE_SET(triggering_controller, uint8, Number, message);
        READ_VALUE_SET(triggering_location, uint8, Number, message);
        READ_VALUE_SET(triggering_sequence, uint32, Number, message);
        READ_VALUE_SET(description, uint64, BigInt, message);
        READ_VALUE_SET(chain_size, uint32, Number, message);
        break;
    }
    case MSG_CHAINED: {
        READ_VALUE_SET(chain_size, uint32, Number, message);
        break;
    }
    case MSG_CHAIN_SOLVING: {
        READ_VALUE_SET(chain_size, uint32, Number, message);
        break;
    }
    case MSG_CHAIN_SOLVED: {
        READ_VALUE_SET(chain_size, uint32, Number, message);
        break;
    }
    case MSG_CHAIN_END: {
        break;
    }
    case MSG_CHAIN_NEGATED: {
        READ_VALUE_SET(chain_size, uint32, Number, message);
        break;
    }
    case MSG_CHAIN_DISABLED: {
        READ_VALUE_SET(chain_size, uint32, Number, message);
        break;
    }
    case MSG_CARD_SELECTED: {
        READ_VALUE(cardsLength, uint32);
        Array cards = Array::New(env, cardsLength);
        for (uint32 i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            READ_VALUE_SET(position, uint32, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_RANDOM_SELECTED: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE(cardsLength, uint32);
        Array cards = Array::New(env, cardsLength);
        for (uint32 i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            READ_VALUE_SET(position, uint32, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_BECOME_TARGET: {
        READ_VALUE(cardsLength, uint32);
        Array cards = Array::New(env, cardsLength);
        for (uint32 i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            READ_VALUE_SET(position, uint32, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    case MSG_DRAW: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE(drawnLength, uint32);
        Array drawn = Array::New(env, drawnLength);
        for (uint32 i = 0; i < drawnLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(position, uint32, Number, value);
            drawn.Set(i, value);
        }
        message.Set("drawn", drawn);
        break;
    }
    case MSG_DAMAGE: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(amount, uint32, Number, message);
        break;
    }
    case MSG_RECOVER: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(amount, uint32, Number, message);
        break;
    }
    case MSG_EQUIP: {
        {
            Object card = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, card);
            READ_VALUE_SET(location, uint8, Number, card);
            READ_VALUE_SET(sequence, uint32, Number, card);
            READ_VALUE_SET(position, uint32, Number, card);
            message.Set("card", card);
        }
        {
            Object target = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, target);
            READ_VALUE_SET(location, uint8, Number, target);
            READ_VALUE_SET(sequence, uint32, Number, target);
            READ_VALUE_SET(position, uint32, Number, target);
            message.Set("target", target);
        }
        break;
    }
    case MSG_LPUPDATE: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(lp, uint32, Number, message);
        break;
    }
    case MSG_UNEQUIP: {
        break;
    }
    case MSG_CARD_TARGET: {
        {
            Object card = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, card);
            READ_VALUE_SET(location, uint8, Number, card);
            READ_VALUE_SET(sequence, uint32, Number, card);
            READ_VALUE_SET(position, uint32, Number, card);
            message.Set("card", card);
        }
        {
            Object target = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, target);
            READ_VALUE_SET(location, uint8, Number, target);
            READ_VALUE_SET(sequence, uint32, Number, target);
            READ_VALUE_SET(position, uint32, Number, target);
            message.Set("target", target);
        }
        break;
    }
    case MSG_CANCEL_TARGET: {
        {
            Object card = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, card);
            READ_VALUE_SET(location, uint8, Number, card);
            READ_VALUE_SET(sequence, uint32, Number, card);
            READ_VALUE_SET(position, uint32, Number, card);
            message.Set("card", card);
        }
        {
            Object target = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, target);
            READ_VALUE_SET(location, uint8, Number, target);
            READ_VALUE_SET(sequence, uint32, Number, target);
            READ_VALUE_SET(position, uint32, Number, target);
            message.Set("target", target);
        }
        break;
    }
    case MSG_PAY_LPCOST: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(amount, uint32, Number, message);
        break;
    }
    case MSG_ADD_COUNTER: {
        READ_VALUE_SET(counter_type, uint16, Number, message);
        READ_VALUE_SET(controller, uint8, Number, message);
        READ_VALUE_SET(location, uint8, Number, message);
        READ_VALUE_SET(sequence, uint8, Number, message);
        READ_VALUE_SET(count, uint16, Number, message);
        break;
    }
    case MSG_REMOVE_COUNTER: {
        READ_VALUE_SET(counter_type, uint16, Number, message);
        READ_VALUE_SET(controller, uint8, Number, message);
        READ_VALUE_SET(location, uint8, Number, message);
        READ_VALUE_SET(sequence, uint8, Number, message);
        READ_VALUE_SET(count, uint16, Number, message);
        break;
    }
    case MSG_ATTACK: {
        {
            Object card = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, card);
            READ_VALUE_SET(location, uint8, Number, card);
            READ_VALUE_SET(sequence, uint32, Number, card);
            READ_VALUE_SET(position, uint32, Number, card);
            message.Set("card", card);
        }
        {
            READ_VALUE(controller, uint8);
            READ_VALUE(location, uint8);
            READ_VALUE(sequence, uint32);
            READ_VALUE(position, uint32);
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
            READ_VALUE_SET(controller, uint8, Number, card);
            READ_VALUE_SET(location, uint8, Number, card);
            READ_VALUE_SET(sequence, uint32, Number, card);
            READ_VALUE_SET(position, uint32, Number, card);
            READ_VALUE_SET(attack, uint32, Number, card);
            READ_VALUE_SET(defense, uint32, Number, card);
            READ_VALUE_SET(destroyed, uint8, Boolean, card);
            message.Set("card", card);
        }
        {
            READ_VALUE(controller, uint8);
            READ_VALUE(location, uint8);
            READ_VALUE(sequence, uint32);
            READ_VALUE(position, uint32);
            READ_VALUE(attack, uint32);
            READ_VALUE(defense, uint32);
            READ_VALUE(destroyed, uint8);
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
        READ_VALUE_SET(controller, uint8, Number, message);
        READ_VALUE_SET(location, uint8, Number, message);
        READ_VALUE_SET(sequence, uint32, Number, message);
        READ_VALUE_SET(position, uint32, Number, message);
        READ_VALUE_SET(code, uint32, Number, message);
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
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE(resultsLength, uint8);
        Array results = Array::New(env, resultsLength);
        for (uint8 i = 0; i < resultsLength; i++) {
            READ_VALUE(result, uint8);
            results.Set(i, Boolean::New(env, result));
        }
        message.Set("results", results);
        break;
    }
    case MSG_TOSS_DICE: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE(resultsLength, uint8);
        Array results = Array::New(env, resultsLength);
        for (uint8 i = 0; i < resultsLength; i++) {
            READ_VALUE(result, uint8);
            results.Set(uint32(i), Number::New(env, result));
        }
        message.Set("results", results);
        break;
    }
    case MSG_ROCK_PAPER_SCISSORS: {
        READ_VALUE_SET(player, uint8, Number, message);
        break;
    }
    case MSG_HAND_RES: {
        READ_VALUE(result, uint8);
        Array results = Array::New(env, 2);
        results.Set(uint32(0), Number::New(env, result & 0b11));
        results.Set(uint32(1), Number::New(env, (result >> 2) & 0b11));
        message.Set("results", results);
        break;
    }
    case MSG_ANNOUNCE_RACE: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(count, uint8, Number, message);
        READ_VALUE_SET(available, uint8, Number, message);
        break;
    }
    case MSG_ANNOUNCE_ATTRIB: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(count, uint8, Number, message);
        READ_VALUE_SET(available, uint8, Number, message);
        break;
    }
    case MSG_ANNOUNCE_CARD: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE(opcodesLength, uint8);
        Array opcodes = Array::New(env, opcodesLength);
        for (uint8 i = 0; i < opcodesLength; i++) {
            READ_VALUE(opcode, uint64);
            opcodes.Set(uint32(i), BigInt::New(env, opcode));
        }
        message.Set("opcodes", opcodes);
        break;
    }
    case MSG_ANNOUNCE_NUMBER: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE(optionsLength, uint8);
        Array options = Array::New(env, optionsLength);
        for (uint8 i = 0; i < optionsLength; i++) {
            READ_VALUE(option, uint64);
            options.Set(uint32(i), Number::New(env, option));
        }
        message.Set("options", options);
        break;
    }
    case MSG_CARD_HINT: {
        READ_VALUE_SET(controller, uint8, Number, message);
        READ_VALUE_SET(location, uint8, Number, message);
        READ_VALUE_SET(sequence, uint32, Number, message);
        READ_VALUE_SET(position, uint32, Number, message);
        READ_VALUE_SET(card_hint, uint8, Number, message);
        READ_VALUE_SET(description, uint64, BigInt, message);
        break;
    }
    case MSG_TAG_SWAP: {
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(deck_size, uint32, Number, message);
        READ_VALUE(extraLength, uint32);
        READ_VALUE_SET(extra_faceup_count, uint32, Number, message);
        READ_VALUE(handLength, uint32);
        READ_VALUE_VSET(deck_top_card, uint32,
                        deck_top_card == 0 ? env.Null()
                                           : Number::New(env, deck_top_card),
                        message);

        Array hand = Array::New(env, handLength);
        for (uint32 i = 0; i < handLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(position, uint32, Number, value);
            hand.Set(i, value);
        }
        message.Set("hand", hand);

        Array extra = Array::New(env, extraLength);
        for (uint32 i = 0; i < extraLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
            READ_VALUE_SET(position, uint32, Number, value);
            extra.Set(i, value);
        }
        message.Set("extra", extra);
        break;
    }
    case MSG_RELOAD_FIELD: {
        READ_VALUE_SET(flags, uint32, Number, message);
        Array players = Array::New(env, 2);
        for (uint32 i = 0; i < 2; i++) {
            Object player = Object::New(env);
            READ_VALUE_SET(lp, uint32, Number, player);
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
        message.Set("players", players);

        READ_VALUE(chainLength, uint32);
        Array chain = Array::New(env, chainLength);
        for (uint32 i = 0; i < chainLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(code, uint32, Number, value);
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
        READ_VALUE_SET(player, uint8, Number, message);
        READ_VALUE_SET(player_hint, uint8, Number, message);
        READ_VALUE_SET(description, uint64, BigInt, message);
        break;
    }
    case MSG_MATCH_KILL: {
        READ_VALUE_SET(card, uint32, Number, message);
        break;
    }
    case MSG_CUSTOM_MSG: {
        break;
    }
    case MSG_REMOVE_CARDS: {
        READ_VALUE(cardsLength, uint32);
        Array cards = Array::New(env, cardsLength);
        for (uint32 i = 0; i < cardsLength; i++) {
            Object value = Object::New(env);
            READ_VALUE_SET(controller, uint8, Number, value);
            READ_VALUE_SET(location, uint8, Number, value);
            READ_VALUE_SET(sequence, uint32, Number, value);
            READ_VALUE_SET(position, uint32, Number, value);
            cards.Set(i, value);
        }
        message.Set("cards", cards);
        break;
    }
    }
    return message;
}
