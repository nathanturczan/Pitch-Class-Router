// ===== CONFIG =====

// Name (or substring) of the IAC port that Scale Navigator uses.
const PORT_NAME_MATCH = "INTERSTICES";

// Incoming scale definition from Scale Navigator:
const SCALE_IN_CHANNEL = 7; // 1–16

// Outgoing control channel for plugin mapping:
const OUT_CHANNEL = 8; // 1–16, set Live to listen to this

// First CC number to use for pitch-class toggles (C..B).
// C -> CC20, C# -> CC21, ..., B -> CC31 by default.
const BASE_CC = 20;

// Pitch class names (for the buttons).
const PC_NAMES = ["C", "C♯/D♭", "D", "D♯/E♭", "E", "F",
                  "F♯/G♭", "G", "G♯/A♭", "A", "A♯/B♭", "B"];

// ===== STATE =====

let midiAccess = null;
let scaleInput = null;   // input from Scale Navigator
let midiOutput = null;   // output to DAW / plugin

// current on/off state for each pitch class
const pcState = new Array(12).fill(false);

// ===== UI SETUP =====

const statusEl = document.getElementById("status");
const buttonsContainer = document.getElementById("mapping-buttons");

function setStatus(msg) {
  console.log("[PitchClassRouter]", msg);
  statusEl.textContent = msg;
}

function createMappingButtons() {
  PC_NAMES.forEach((name, idx) => {
    const btn = document.createElement("button");
    btn.className = "pc-btn";
    btn.textContent = `${idx}: ${name}`;
    btn.addEventListener("click", () => {
      // Send "ON" value so Live can learn this CC.
      sendPitchClassCC(idx, true);
    });
    buttonsContainer.appendChild(btn);
  });
}

// ===== MIDI SETUP =====

if (!navigator.requestMIDIAccess) {
  setStatus("WebMIDI not supported in this browser.");
} else {
  navigator.requestMIDIAccess({ sysex: false })
    .then(onMIDISuccess, onMIDIFailure);
}

function onMIDISuccess(access) {
  midiAccess = access;
  createMappingButtons();
  findPorts();

  midiAccess.onstatechange = () => {
    // Re-scan ports if devices change
    findPorts();
  };
}

function onMIDIFailure(err) {
  console.error("MIDI init failed:", err);
  setStatus("Could not access MIDI. Check browser permissions.");
}

function findPorts() {
  scaleInput = null;
  midiOutput = null;

  for (const input of midiAccess.inputs.values()) {
    if (input.name && input.name.includes(PORT_NAME_MATCH)) {
      scaleInput = input;
      scaleInput.onmidimessage = handleScaleMessage;
    }
  }

  for (const output of midiAccess.outputs.values()) {
    if (output.name && output.name.includes(PORT_NAME_MATCH)) {
      midiOutput = output;
    }
  }

  if (scaleInput && midiOutput) {
    setStatus(
      `Connected. Listening on "${scaleInput.name}" ch ${SCALE_IN_CHANNEL},` +
      ` sending on "${midiOutput.name}" ch ${OUT_CHANNEL}.`
    );
  } else if (!scaleInput && !midiOutput) {
    setStatus(`No MIDI ports matching "${PORT_NAME_MATCH}" found.`);
  } else if (!scaleInput) {
    setStatus(`Output found but no input named "${PORT_NAME_MATCH}".`);
  } else {
    setStatus(`Input found but no output named "${PORT_NAME_MATCH}".`);
  }
}

// ===== MIDI HELPERS =====

function sendCC(ccNumber, value) {
  if (!midiOutput) return;
  const status = 0xB0 + (OUT_CHANNEL - 1); // Control Change on OUT_CHANNEL
  midiOutput.send([status, ccNumber & 0x7f, value & 0x7f]);
}

function sendPitchClassCC(pcIndex, isOn) {
  const ccNumber = BASE_CC + (pcIndex % 12);
  const value = isOn ? 127 : 0;
  sendCC(ccNumber, value);
}

// ===== INCOMING SCALE HANDLER =====

// Contract for incoming messages (from Scale Navigator):
// - Port:  INTERSTICES
// - Channel: 7 (SCALE_IN_CHANNEL)
// - Note number: any; we'll use note % 12 as pitch class.
// - Velocity > 0  => pitch class is IN the scale.
// - Velocity == 0 or NoteOff => pitch class is OUT of the scale.

function handleScaleMessage(event) {
  const [status, note, velocity] = event.data;

  const msgType = status & 0xf0;
  const channel = (status & 0x0f) + 1; // 1–16

  // Only care about Note On / Note Off on the configured channel.
  if (channel !== SCALE_IN_CHANNEL) return;
  if (msgType !== 0x90 && msgType !== 0x80) return;

  const pcIndex = note % 12;

  let isOn;
  if (msgType === 0x80 || velocity === 0) {
    // Note Off
    isOn = false;
  } else {
    // Note On with velocity > 0
    isOn = true;
  }

  if (pcState[pcIndex] === isOn) {
    // No change, nothing to send.
    return;
  }

  pcState[pcIndex] = isOn;
  sendPitchClassCC(pcIndex, isOn);
}
