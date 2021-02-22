import {
    OcgAttribute,
    OcgDuelMode,
    OcgLinkMarker,
    OcgLocation,
    OcgLogType,
    OcgPosition,
    OcgProcessResult,
    OcgRace,
    OcgType,
} from "./type_core";
import { OcgMessage } from "./type_message";
import { OcgQuery, OcgQueryField } from "./type_query";

export * from "./type_message";
export * from "./type_core";

let native: any = null;
if (process.platform == "win32" && process.arch == "x64") {
    native = require(__dirname + "/../lib/native-win64.node");
} else if (process.platform == "win32" && process.arch == "ia32") {
    native = require(__dirname + "/../lib/native-win32.node");
} else {
    native = require(__dirname + "/../lib/native.node");
}

export interface OcgCardData {
    code: number;
    alias: number;
    setcodes: number[];
    type: OcgType;
    level: number;
    attribute: OcgAttribute;
    race: OcgRace;
    attack: number;
    defense: number;
    lscale: number;
    rscale: number;
    link_marker: OcgLinkMarker;
}

export interface OcgNewCardInfo {
    team: number;
    duelist: number;
    code: number;
    con: number;
    loc: OcgLocation;
    seq: number;
    pos: OcgPosition;
}

export type OcgDuelHandle = object;

export interface OcgDuelOptions {
    flags: OcgDuelMode;
    seed: number;
    team1: {
        startingLP: number;
        startingDrawCount: number;
        drawCountPerTurn: number;
    };
    team2: {
        startingLP: number;
        startingDrawCount: number;
        drawCountPerTurn: number;
    };
    cardReader: (card: number) => OcgCardData;
    scriptReader: (name: string) => boolean;
    cardReaderDone?: (card: OcgCardData) => void;
    logHandler?: (text: string, type: OcgLogType) => void;
}

export function getVersion(): [number, number] {
    return native.getVersion();
}

function verifyType(fname: string, name: string, value: any, type: string) {
    if (typeof value !== type) {
        throw new TypeError(
            `${fname}: "${name}" has wrong type, ${type} expected`
        );
    }
}
function verifyTypeIf(fname: string, name: string, value: any, type: string) {
    if (typeof value !== "undefined" && typeof value !== type) {
        throw new TypeError(
            `${fname}: "${name}" has wrong type, ${type} or undefined expected`
        );
    }
}
function verifyTypeArray(fname: string, name: string, value: any) {
    if (!Array.isArray(value)) {
        throw new TypeError(
            `${fname}: "${name}" has wrong type, array expected`
        );
    }
}

function createCardReader(cardReader: OcgDuelOptions["cardReader"]) {
    return (id: number) => {
        const card = cardReader(id);
        verifyType("cardReader", "card.code", card.code, "number");
        verifyType("cardReader", "card.alias", card.alias, "number");
        verifyTypeArray("cardReader", "card.setcodes", card.setcodes);
        verifyType("cardReader", "card.type", card.type, "number");
        verifyType("cardReader", "card.level", card.level, "number");
        verifyType("cardReader", "card.attribute", card.attribute, "number");
        verifyType("cardReader", "card.race", card.race, "number");
        verifyType("cardReader", "card.attack", card.attack, "number");
        verifyType("cardReader", "card.defense", card.defense, "number");
        verifyType("cardReader", "card.lscale", card.lscale, "number");
        verifyType("cardReader", "card.rscale", card.rscale, "number");
        verifyType(
            "cardReader",
            "card.link_marker",
            card.link_marker,
            "number"
        );
        for (let i = 0; i < card.setcodes.length; i++) {
            verifyType(
                "cardReader",
                `card.setcodes[${i}]`,
                card.setcodes[i],
                "number"
            );
        }
        return card;
    };
}

export function createDuel(options: OcgDuelOptions): OcgDuelHandle {
    verifyType("createDuel", "options.flags", options.flags, "bigint");
    verifyType("createDuel", "options.seed", options.seed, "number");
    verifyType("createDuel", "options.team1", options.team1, "object");
    verifyType(
        "createDuel",
        "options.team1.startingLP",
        options.team1.startingLP,
        "number"
    );
    verifyType(
        "createDuel",
        "options.team1.startingDrawCount",
        options.team1.startingDrawCount,
        "number"
    );
    verifyType(
        "createDuel",
        "options.team1.drawCountPerTurn",
        options.team1.drawCountPerTurn,
        "number"
    );
    verifyType("createDuel", "options.team2", options.team2, "object");
    verifyType(
        "createDuel",
        "options.team2.startingLP",
        options.team2.startingLP,
        "number"
    );
    verifyType(
        "createDuel",
        "options.team2.startingDrawCount",
        options.team2.startingDrawCount,
        "number"
    );
    verifyType(
        "createDuel",
        "options.team2.drawCountPerTurn",
        options.team2.drawCountPerTurn,
        "number"
    );
    verifyType(
        "cardReader",
        "options.cardReader",
        options.cardReader,
        "function"
    );
    verifyType(
        "cardReader",
        "options.scriptReader",
        options.scriptReader,
        "function"
    );
    verifyTypeIf(
        "cardReader",
        "options.cardReaderDone",
        options.cardReaderDone,
        "function"
    );
    verifyTypeIf(
        "cardReader",
        "options.logHandler",
        options.logHandler,
        "function"
    );

    return native.createDuel({
        flags: options.flags,
        seed: options.seed,
        team1_startingLP: options.team1.startingLP,
        team1_startingDrawCount: options.team1.startingDrawCount,
        team1_drawCountPerTurn: options.team1.drawCountPerTurn,
        team2_startingLP: options.team2.startingLP,
        team2_startingDrawCount: options.team2.startingDrawCount,
        team2_drawCountPerTurn: options.team2.drawCountPerTurn,
        scriptReader: options.scriptReader,
        cardReader: createCardReader(options.cardReader),
        cardReaderDone: !options.cardReaderDone ? undefined : () => {},
        logHandler: !options.logHandler ? undefined : options.logHandler,
    });
}

export function loadScript(
    duel: OcgDuelHandle,
    file: string,
    name: string
): OcgDuelHandle {
    return native.loadScript(duel, file, name);
}

export function duelNewCard(duel: OcgDuelHandle, card: OcgNewCardInfo) {
    verifyType("duelNewCard", "duel", duel, "object");
    verifyType("duelNewCard", "card", card, "object");
    verifyType("duelNewCard", "card.team", card.team, "number");
    verifyType("duelNewCard", "card.duelist", card.duelist, "number");
    verifyType("duelNewCard", "card.code", card.code, "number");
    verifyType("duelNewCard", "card.con", card.con, "number");
    verifyType("duelNewCard", "card.loc", card.loc, "number");
    verifyType("duelNewCard", "card.seq", card.seq, "number");
    verifyType("duelNewCard", "card.pos", card.pos, "number");

    native.duelNewCard(duel, {
        team: card.team,
        duelist: card.duelist,
        code: card.code,
        con: card.con,
        loc: card.loc,
        seq: card.seq,
        pos: card.pos,
    });
}

export function startDuel(duel: OcgDuelHandle) {
    verifyType("startDuel", "duel", duel, "object");
    native.startDuel(duel);
}

export function duelProcess(duel: OcgDuelHandle): OcgProcessResult {
    verifyType("duelProcess", "duel", duel, "object");
    return native.duelProcess(duel);
}

export function duelGetMessage(duel: OcgDuelHandle): OcgMessage[] {
    verifyType("duelProcess", "duel", duel, "object");
    return native.duelGetMessage(duel);
}

export function duelQueryCount(
    duel: OcgDuelHandle,
    player: number,
    location: OcgLocation
): number {
    verifyType("duelQueryCount", "duel", duel, "object");
    verifyType("duelQueryCount", "player", duel, "number");
    verifyType("duelQueryCount", "location", duel, "number");
    return native.duelQueryCount(duel, player, location);
}

export function duelQuery(
    duel: OcgDuelHandle,
    controller: number,
    location: OcgLocation,
    sequence: number,
    overlayIndex?: number
): OcgQuery {
    verifyType("duelQuery", "duel", duel, "object");
    verifyType("duelQuery", "controller", controller, "number");
    verifyType("duelQuery", "location", location, "number");
    verifyType("duelQuery", "sequence", sequence, "number");
    verifyTypeIf("duelQuery", "overlayIndex", overlayIndex, "number");
    return native.duelQuery(duel, controller, location, sequence, overlayIndex);
}

export function duelQueryLocation(
    duel: OcgDuelHandle,
    controller: number,
    location: OcgLocation
): OcgQuery[] {
    verifyType("duelQuery", "duel", duel, "object");
    verifyType("duelQuery", "controller", controller, "number");
    verifyType("duelQuery", "location", location, "number");
    return native.duelQueryLocation(duel, controller, location);
}

export function duelQueryField(duel: OcgDuelHandle): OcgQueryField {
    verifyType("duelQueryField", "duel", duel, "object");
    return native.duelQueryField(duel);
}
