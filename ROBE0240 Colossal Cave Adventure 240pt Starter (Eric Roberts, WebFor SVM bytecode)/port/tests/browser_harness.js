/*
 * browser_harness.js - drive the original browser edition (index.html of
 * ..\..\..\ROBE0665 ...\archive_original\Browser, which carries the js\
 * run-time; serve it with  python -m http.server 8765 --directory <Browser>)
 * from the page's console / a DevTools snippet, so that its transcript can
 * be compared byte for byte with  starter.exe --seed N --no-fixes --echo.
 *
 *   1. load the page, paste this file, then  await harnessInstall(11)
 *   2. harnessStart('Starter')   (or 'Wellesley')  and wait ~4 s
 *   3. harnessRun([...commands...])  -> number of commands the game took
 *   4. await harnessDigest()     -> { sha, bytes, text }
 *
 * Synthetic key events do not reach JSConsole's keypress handler reliably,
 * so harnessRun feeds each line exactly as the Enter branch of
 * JSConsole.keypress does: set console.input, append the line and <br>,
 * fireActionListeners().  Math.random is replaced by the splitmix64
 * generator starter.exe uses, seeded the same way.
 */

async function harnessInstall(seed) {
    await new Promise(res => require(['edu/stanford/cs/jsconsole', 'edu/stanford/cs/svm'], (jc, sv) => {
        const p = jc.JSConsole.prototype, show = p.showErrorMessage;
        p.showErrorMessage = function (m) { (window.__errs = window.__errs || []).push(String(m)); return show.call(this, m); };
        const run = sv.SVM.prototype.run;
        sv.SVM.prototype.run = function () { window.__svm = this; return run.call(this); };
        res(true);
    }));
    let st = BigInt(seed);
    const M = (1n << 64n) - 1n;
    Math.random = function () {
        st = (st + 0x9E3779B97F4A7C15n) & M;
        let z = st;
        z = ((z ^ (z >> 30n)) * 0xBF58476D1CE4E5B9n) & M;
        z = ((z ^ (z >> 27n)) * 0x94D049BB133111EBn) & M;
        z = z ^ (z >> 31n);
        return Number(z >> 11n) / 9007199254740992;
    };
}

function harnessStart(which) {
    document.getElementById(which).click();
}

function harnessRun(cmds) {
    let n = 0;
    for (const s of cmds) {
        if (window.__svm.getState() !== 6) break;          /* 6 = WAITING for input */
        const c = window.__svm.getConsole();
        c.input = s;
        c.element.innerHTML += s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/ /g, '&nbsp;') + '<br>';
        c.fireActionListeners();
        n++;
    }
    return n;
}

async function harnessDigest() {
    const t = document.createElement('textarea');
    t.innerHTML = window.__svm.getConsole().element.innerHTML
        .replace(/<br\s*\/?>/gi, '\n').replace(/<[^>]+>/g, '');
    const text = t.value.replace(/ /g, ' ');
    const buf = new TextEncoder().encode(text);
    const d = await crypto.subtle.digest('SHA-256', buf);
    return { sha: Array.from(new Uint8Array(d)).map(b => b.toString(16).padStart(2, '0')).join(''),
             bytes: buf.length, errors: window.__errs || null, text };
}
