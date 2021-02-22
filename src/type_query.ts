import {
    OcgAttribute,
    OcgDuelMode,
    OcgLinkMarker,
    OcgPosition,
    OcgRace,
    OcgType,
} from "./type_core";
import { OcgChain, OcgFieldPlayer, OcgLocPos } from "./type_message";

export interface OcgQueryInfo {}

export interface OcgQuery {
    code: number;
    position: OcgPosition;
    alias: number;
    type: OcgType;
    level: number;
    rank: number;
    attribute: OcgAttribute;
    race: OcgRace;
    attack: number;
    defense: number;
    base_attack: number;
    base_defence: number;
    reason: number;
    cover: number;
    owner: number;
    status: number;
    is_public: number;
    is_hidden: number;
    lscale: number;
    rscale: number;
    reason_card: OcgLocPos | null;
    equip_card: OcgLocPos | null;
    target_cards: OcgLocPos[];
    overlay_cards: number[];
    counters: {
        counter: number;
        count: number;
    }[];
    link: {
        link: number;
        markers: OcgLinkMarker;
    };
}

export interface OcgQueryField {
    flags: OcgDuelMode;
    players: [OcgFieldPlayer, OcgFieldPlayer];
    chain: OcgChain[];
}
