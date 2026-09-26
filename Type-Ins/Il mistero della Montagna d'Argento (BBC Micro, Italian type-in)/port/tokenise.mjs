// Tokenise a BBC BASIC text listing using the real BASIC ROM, via jsbeeb.
//
//   JSBEEB=/path/to/jsbeeb node tokenise.mjs montagna.bas montagna.tok
//
// jsbeeb: git clone https://github.com/mattgodbolt/jsbeeb && cd jsbeeb && npm install
import { readFileSync, writeFileSync } from "fs";
import { pathToFileURL } from "url";
import path from "path";

const JSBEEB = path.resolve(process.env.JSBEEB || "./jsbeeb");
const { create } = await import(pathToFileURL(path.join(JSBEEB, "src/basic-tokenise.js")).href);
const { setNodeBasePath } = await import(pathToFileURL(path.join(JSBEEB, "src/loader.js")).href);
setNodeBasePath(JSBEEB);

const [, , inFile, outFile] = process.argv;
const src = readFileSync(inFile, "utf8").replace(/\r\n/g, "\n");
const t = await create();
const out = t.tokenise(src);
const buf = Buffer.alloc(out.length);
for (let i = 0; i < out.length; i++) buf[i] = out.charCodeAt(i) & 0xff;
writeFileSync(outFile, buf);
console.log(`tokenised ${src.split("\n").filter((l) => l.trim()).length} lines -> ${buf.length} bytes`);
