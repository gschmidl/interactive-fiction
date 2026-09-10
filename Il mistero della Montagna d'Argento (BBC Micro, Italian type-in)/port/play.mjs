// Headless BBC Micro test-drive of MONTAGNA.ssd (SHIFT+BREAK autoboot).
//
//   JSBEEB=/path/to/jsbeeb node play.mjs "1" "E" "PRENDI PANE" "INVENTARIO"
//
// Each argument is one line typed at the game's prompt.  After every command
// the reconstructed 40-column screen is printed, so the run doubles as a
// transcript.
import { pathToFileURL } from "url";
import path from "path";

const JSBEEB = path.resolve(process.env.JSBEEB || "./jsbeeb");
const imp = async (p) => await import(pathToFileURL(path.join(JSBEEB, p)).href);
const { MachineSession } = await imp("src/machine-session.js");
const { BBC } = await imp("src/keymap.js");

const disc = process.env.DISC || "./MONTAGNA.ssd";
const s = new MachineSession("B-DFS1.2", { discImage: path.resolve(disc) });
await s.initialise();

// SHIFT+BREAK: hold SHIFT across a hard reset so the DFS boot option runs !BOOT.
s.keyDownRaw(BBC.SHIFT);
s.reset(true);
await s.runFor(2_000_000);
s.releaseAllKeys();

const settle = async () => {
    await s.runUntilPrompt(60, { clear: false });
    await s.runFor(4_000_000);
};

const show = (label) => {
    const out = s.drainOutput({ clear: true });
    const txt = out.screenText.replace(/[ \t]+$/gm, "").replace(/\n{3,}/g, "\n\n").trim();
    process.stdout.write("\n" + "=".repeat(46) + "\n>>> " + label + "\n" + txt + "\n");
};

await settle();
show("(boot)");

for (const c of process.argv.slice(2)) {
    await s.type(c); // type() supplies the newline itself
    await settle();
    show(c);
}
s.destroy();
