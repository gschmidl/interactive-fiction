#!/usr/bin/env node
// Submit a card deck to the running NOS 1.3 and write everything the job
// printed to a file.   node job.js <deck> <output> [timeoutSeconds]
const fs = require("fs");
const DtCyber = require("../automation/DtCyber");

const deck = process.argv[2];
const outf = process.argv[3] || "job.out";
const secs = parseInt(process.argv[4] || "300");

const dtc = new DtCyber();
let buf = "";

function done(code) {
  fs.writeFileSync(outf, buf);
  console.log(`${buf.length} bytes written to ${outf}`);
  process.exit(code);
}

setTimeout(() => { console.log("timed out"); done(2); }, secs * 1000);

dtc.connect()
.then(() => dtc.expect([{ re: /Operator> $/ }]))
.then(() => dtc.attachPrinter("LP5xx_C11_E5"))
.then(() => dtc.runJob(11, 4, deck, data => { buf += data.toString("latin1"); }))
.then(() => done(0))
.catch(err => { console.log(err.message ? err.message : err); done(1); });
