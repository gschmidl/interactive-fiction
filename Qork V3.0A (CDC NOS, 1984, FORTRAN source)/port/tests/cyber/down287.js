// Shut the running NOS 2.8.7 down gracefully and stop DtCyber.
const DtCyber = require("../automation/DtCyber");
const dtc = new DtCyber();
setTimeout(() => { console.log("timed out"); process.exit(2); }, 240000);
dtc.connect()
.then(() => dtc.expect([{ re: /Operator> $/ }]))
.then(() => dtc.shutdown(false))
.then(() => { console.log("NOS 2.8.7 shut down"); process.exit(0); })
.catch(err => { console.log(err.message || err); process.exit(1); });
