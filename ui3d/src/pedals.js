// Pedal catalogue: same ids, names, ranges, defaults and skew as plugin/Params.cpp.
// p(id, name, min, max, def, centre, unit, choices, isBool)
const p = (id, name, min, max, def, centre = 0, unit = "", choices = null, isBool = false) => ({ id, name, min, max, def, centre, unit, choices, isBool });

export const PEDALS = [
  { id: "gate", name: "Noise Gate", short: "GATE", color: 0x1d1f24, ink: "#39ff14", w: 0.074,
    desc: "Taglia il fruscio quando non suoni: si apre sopra la soglia e si richiude 6 dB sotto (niente tremolii).",
    params: [p("thresh", "Threshold", -90, -10, -62, 0, "dB"), p("release", "Release", 5, 500, 80, 60, "ms")] },
  { id: "comp", name: "Sustainer", short: "SUSTAINER", color: 0x1b4fa8, ink: "#e9f1ff", w: 0.074,
    desc: "Compressore: abbassa i picchi e alza le code, le note durano di più e il suono è più uniforme.",
    params: [p("sustain", "Sustain", 0, 1, 0.5), p("attack", "Attack", 0.5, 50, 8, 8, "ms"), p("level", "Level", -12, 12, 0, 0, "dB"), p("blend", "Blend", 0, 1, 1)] },
  { id: "boost", name: "Treble Booster", short: "TREBLE BOOST", color: 0x9a9c9e, ink: "#141414", w: 0.074, metal: true,
    desc: "Booster al germanio: lascia passare solo medio-alti e li spinge nell'ampli. Il lead 'cantato' anni '70.",
    params: [p("amount", "Boost", 0, 1, 0.6), p("range", "Range", 0, 2, 0, 0, "", ["Treble", "Mid", "Full"]), p("level", "Level", -24, 12, 0, 0, "dB")] },
  { id: "od", name: "Green Drive", short: "GREEN DRIVE", color: 0x1f8a3a, ink: "#f2fff0", w: 0.074,
    desc: "Overdrive tipo Tube Screamer: i bassi passano puliti, i medi vengono spinti e saturati. Stringe il suono.",
    params: [p("drive", "Drive", 0, 1, 0.4), p("tone", "Tone", 0, 1, 0.5), p("level", "Level", -24, 12, 0, 0, "dB")] },
  { id: "fuzz", name: "Toxic Fuzz", short: "TOXIC FUZZ", color: 0x5a1d8f, ink: "#39ff14", w: 0.074,
    desc: "Fuzz a transistor: guadagno enorme e clipping duro. Bias 'affama' il transistor per un fuzz sporco e gated.",
    params: [p("amount", "Fuzz", 0, 1, 0.7), p("tone", "Tone", 0, 1, 0.5), p("level", "Level", -24, 12, -6, 0, "dB"), p("bias", "Bias", 0, 1, 0)] },
  { id: "eq", name: "Graphic EQ", short: "GRAPHIC EQ", color: 0xd8d6cf, ink: "#16161a", w: 0.12, sliders: true,
    desc: "Equalizzatore a 7 bande (100 Hz - 6.4 kHz). Prima dell'ampli cambia cosa satura, dopo cambia solo il timbro.",
    params: ["100", "200", "400", "800", "1.6k", "3.2k", "6.4k"].map((n, i) => p("b" + (i + 1), n, -12, 12, 0, 0, "dB")).concat([p("level", "Level", -12, 12, 0, 0, "dB")]) },
  { id: "chorus", name: "Chorus", short: "CHORUS", color: 0x23a6c9, ink: "#05131a", w: 0.074,
    desc: "Copie del suono leggermente ritardate e modulate: effetto 'più chitarre insieme', allarga lo stereo.",
    params: [p("rate", "Rate", 0.05, 8, 0.8, 1, "Hz"), p("depth", "Depth", 0, 1, 0.5), p("mix", "Mix", 0, 1, 0.5)] },
  { id: "phaser", name: "Phaser", short: "PHASER", color: 0xe0661a, ink: "#1b0c02", w: 0.074,
    desc: "Filtri allpass che si muovono: creano 'buchi' nello spettro che scorrono su e giù (suono che gira).",
    params: [p("rate", "Rate", 0.02, 8, 0.4, 1, "Hz"), p("depth", "Depth", 0, 1, 0.8), p("feedback", "Feedback", 0, 1, 0.4), p("mix", "Mix", 0, 1, 1)] },
  { id: "trem", name: "Tremolo", short: "TREMOLO", color: 0x7a1420, ink: "#ffe9d0", w: 0.074,
    desc: "Il volume sale e scende a tempo. Shape va da onda morbida (vintage) a quadra (stacco netto).",
    params: [p("rate", "Rate", 0.5, 15, 5, 4, "Hz"), p("depth", "Depth", 0, 1, 0.6), p("shape", "Shape", 0, 1, 0.2)] },
  { id: "delay", name: "Delay", short: "DELAY", color: 0x2b2f6e, ink: "#c9b8ff", w: 0.12,
    desc: "Eco con ripetizioni che si scuriscono come un analogico. Sync lo aggancia al tempo di FL Studio.",
    params: [p("time", "Time", 10, 2000, 380, 400, "ms"), p("sync", "Sync", 0, 1, 0, 0, "", null, true),
      p("div", "Division", 0, 6, 5, 0, "", ["1/2", "1/4", "1/4 .", "1/4 T", "1/8", "1/8 .", "1/16"]),
      p("feedback", "Feedback", 0, 0.95, 0.35), p("mix", "Mix", 0, 1, 0.25), p("tone", "Tone", 0, 1, 0.5),
      p("pingpong", "Ping-Pong", 0, 1, 0, 0, "", null, true)] },
  { id: "reverb", name: "Reverb", short: "REVERB", color: 0x101114, ink: "#9b4dff", w: 0.12,
    desc: "Riverbero a rete di ritardi (FDN): stanza o hall. Damping scurisce le code come in un ambiente vero.",
    params: [p("mix", "Mix", 0, 1, 0.25), p("decay", "Decay", 0.2, 12, 2.2, 2, "s"), p("size", "Size", 0, 1, 0.6),
      p("predelay", "Pre-Delay", 0, 200, 20, 40, "ms"), p("damping", "Damping", 0, 1, 0.4)] },
  { id: "nfuzz", name: "Nuclear Fuzz", short: "NUCLEAR FUZZ", color: 0x2a1744, ink: "#d8d0e6", w: 0.12, d: 0.125, custom: "nuke", jacks: "top",
    desc: "Fuzz industriale a due stadi con gate integrato: il gate ascolta la chitarra prima del fuzz e taglia il rumore appena fermi le corde.",
    params: [p("drive", "Drive", 0, 1, 0.7), p("gate", "Gate", 0, 1, 0.4), p("volume", "Volume", -24, 12, -6, 0, "dB")] },
  { id: "bioboost", name: "Bio-Boost", short: "BIO-BOOST", color: 0x8fcf2a, ink: "#0d1407", w: 0.046, d: 0.1, custom: "bio", jacks: "top", metal: true,
    desc: "Boost pulito fino a +20 dB per spingere i soli. CLEAN spinge un po' i medi, TREBLE toglie i bassi per un boost brillante.",
    params: [p("boost", "Boost", 0, 1, 0.5), p("mode", "Mode", 0, 1, 0, 0, "", ["Clean", "Treble"])] },
  { id: "mutant", name: "Mutant Mod", short: "MUTANT MOD", color: 0x4a1f80, ink: "#e8e0ff", w: 0.15, d: 0.12, custom: "mutant", jacks: "top",
    desc: "Modulazione tre-in-uno: Chorus liquido, Flanger con feedback o Vibrato (solo segnale modulato). La valvola pulsa a tempo con l'LFO.",
    params: [{ ...p("mode", "Mode", 0, 2, 0, 0, "", ["Chorus", "Flanger", "Vibrato"]), angles: [-50, 50, -140] }, p("speed", "Speed", 0.05, 10, 0.9, 1, "Hz")] },
];

// Same maths as juce::NormalisableRange with a skew centre.
export function toValue(par, n) {
  if (par.isBool) return n >= 0.5 ? 1 : 0;
  if (par.choices) return Math.round(n * (par.choices.length - 1));
  if (par.centre > 0) {
    const skew = Math.log(0.5) / Math.log((par.centre - par.min) / (par.max - par.min));
    return par.min + (par.max - par.min) * Math.exp(Math.log(Math.max(n, 1e-6)) / skew);
  }
  return par.min + (par.max - par.min) * n;
}
export function toNorm(par, v) {
  if (par.isBool) return v ? 1 : 0;
  if (par.choices) return v / (par.choices.length - 1);
  const t = (v - par.min) / (par.max - par.min);
  if (par.centre > 0) {
    const skew = Math.log(0.5) / Math.log((par.centre - par.min) / (par.max - par.min));
    return Math.pow(t, skew);
  }
  return t;
}
export function fmt(par, v) {
  if (par.isBool) return v ? "ON" : "OFF";
  if (par.choices) return par.choices[v];
  if (par.unit === "ms" && v >= 1000) return (v / 1000).toFixed(2) + " s";
  if (!par.unit) return (v * 10).toFixed(1);
  return (Math.abs(v) < 10 ? v.toFixed(2) : v.toFixed(1)) + " " + par.unit;
}

// Panels: every control listed here exists in the plugin (same parameter id).
const K = (id, label) => ({ id, label });
export const AMPS = [
  { id: "british30", model: 0, name: "British 30", file: "models/british30.json", cab: "British 2x12",
    tags: ["Classe A", "4x EL84", "senza controreazione", "testata + 2x12"],
    text: ["Brillante, 'chime', satura ricco quando lo spingi. Il riferimento è il suono Red Special.", "Legno di noce, pannello in ottone, valvole a vista."],
    knobs: [K("gain", "GAIN"), K("master", "MASTER"), K("bass", "BASS"), K("mid", "MIDDLE"), K("treble", "TREBLE"), K("presence", "CUT"), K("resonance", "DEPTH"), K("sag", "SAG")],
    toggles: [{ id: "bright", n: 2, states: ["OFF", "ON"], label: "BRIGHT" }, { id: "channel", n: 3, states: ["CLEAN", "CRUNCH", "LEAD"], label: "CHANNEL" }], tube: 0x5cff3a },
  { id: "american", model: 1, name: "American Twin", file: "models/american.json", cab: "American 1x12",
    tags: ["Classe AB", "6L6", "combo 1x12"],
    text: ["Pulito cristallino con tanta headroom, bassi pieni e alti a campana.", "Pannello nero, tela argento, manopole numerate."],
    knobs: [K("gain", "VOLUME"), K("bass", "BASS"), K("mid", "MIDDLE"), K("treble", "TREBLE"), K("presence", "PRESENCE"), K("resonance", "DEPTH"), K("sag", "SAG"), K("master", "MASTER")],
    toggles: [{ id: "bright", n: 2, states: ["OFF", "ON"], label: "BRIGHT" }, { id: "channel", n: 3, states: ["CLEAN", "CRUNCH", "LEAD"], label: "CHANNEL" }], tube: 0x5cff3a },
  { id: "modern", model: 2, name: "Modern 5050", file: "models/modern5050.json", cab: "Modern 4x12",
    tags: ["Alto guadagno", "4 stadi", "testata + 2x12 su ruote"],
    text: ["Bassi stretti e distorsione densa per riff veloci e palm mute.", "Acciaio spazzolato, griglia esagonale, valvole a vista."],
    knobs: [K("gain", "GAIN"), K("bass", "BASS"), K("mid", "MIDDLE"), K("treble", "TREBLE"), K("presence", "PRESENCE"), K("resonance", "DEPTH"), K("sag", "SAG"), K("master", "MASTER")],
    toggles: [{ id: "bright", n: 2, states: ["OFF", "ON"], label: "BRIGHT" }, { id: "channel", n: 3, states: ["CLEAN", "CRUNCH", "LEAD"], label: "CHANNEL" }], tube: 0x5cff3a },
  { id: "overlord", model: 3, name: "Overlord", file: "models/overlord.json", cab: "Modern 4x12",
    tags: ["Metal moderno / Djent", "Tight + Attack", "EQ medi 8 bande"],
    text: ["Mostro ad alto guadagno per accordature ribassate. TIGHT stringe i bassi prima della distorsione, ATTACK fa uscire la plettrata.", "L'EQ grafico scolpisce i medi da 150 Hz a 4 kHz (±12 dB)."],
    knobs: [K("master", "VOLUME"), K("gain", "GAIN"), K("attack", "ATTACK"), K("bass", "BASS"), K("treble", "TREBLE"), K("presence", "PRESENCE"), K("resonance", "DEPTH")],
    toggles: [{ id: "tight", n: 2, states: ["OFF", "ON"], label: "TIGHT" }],
    sliders: ["150", "250", "400", "630", "1k", "1.6k", "2.5k", "4k"].map((f, i) => ({ id: "eq" + (i + 1), label: f + " Hz" })), tube: 0x46ff2a },
  { id: "vapor", model: 4, name: "Vapor-30", file: "models/vapor30.json", cab: "American 1x12",
    tags: ["Ambient / Shoegaze", "Crystal Clean", "D/R integrato"],
    text: ["Puliti cristallini e tridimensionali. Il circuito VAPOR satura solo quando plettri forte, BLOOM decide quanto dura.", "D/R aggiunge delay + riverbero in stereo, WIDTH lo allarga. STANDBY e POWER spengono l'ampli."],
    knobs: [K("gain", "GAIN"), K("treble", "TREBLE"), K("master", "VOLUME"), K("mid", "MIDDLE"), K("bass", "BASS"), K("vapor", "VAPOR"), K("bloom", "BLOOM"),
            K("amb", "D/R MIX"), K("resonance", "DEPTH"), K("presence", "PRESENCE"), K("amb_width", "WIDTH")],
    toggles: [{ id: "bright", n: 2, states: ["OFF", "ON"], label: "BRIGHT" }, { id: "standby", n: 2, states: ["OFF", "ON"], label: "STANDBY" }, { id: "power", n: 2, states: ["OFF", "ON"], label: "POWER" }], tube: 0xb050ff },
];

export const CABS = ["British 2x12", "American 1x12", "Modern 4x12", "IR utente"];
export const MICS = [
  { id: "dyn", name: "Dinamico", text: "Medi in faccia, attacco deciso: il classico da palco." },
  { id: "ribbon", name: "Nastro", text: "Alti morbidi e bassi caldi: toglie l'asprezza al gain alto." },
  { id: "cond", name: "Condensatore", text: "Dettaglio e aria: si sente anche la stanza." },
];

export const AMP_CONTROLS_LEGACY = [
  { id: "gain", label: "GAIN" }, { id: "master", label: "MASTER" }, { id: "bass", label: "BASS" }, { id: "mid", label: "MIDDLE" },
  { id: "treble", label: "TREBLE" }, { id: "presence", label: "CUT" }, { id: "resonance", label: "DEPTH" }, { id: "sag", label: "SAG" },
];
