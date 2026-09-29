#!/usr/bin/env node
// Start DtCyber, deadstart NOS 2.8.7, and leave it running in the background.
const DtCyber = require("../automation/DtCyber");

const dtc = new DtCyber();

dtc.start({
  detached: true,
  stdio:    [0, "ignore", 2],
  unref:    true
})
.then(() => dtc.say("DtCyber started - deadstarting NOS 2.8.7"))
.then(() => dtc.sleep(2000))
.then(() => dtc.attachPrinter("LP5xx_C12_E5"))
.then(() => dtc.expect([{ re: /QUEUE FILE UTILITY COMPLETE/ }], "printer"))
.then(() => dtc.say("Deadstart complete - NOS 2.8.7 is up on operator port 6662"))
.then(() => process.exit(0))
.catch(err => { console.log(err); process.exit(1); });
