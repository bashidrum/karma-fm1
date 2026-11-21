// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
//
// Node checks of the web pages' JS (no browser, no hardware). Run from the repo root:
//   node web/test_web.mjs
// - editor.html (PHEBUS: the backup page): its protocol section (between PROTO-BEGIN/END) against a small simulated
//   FM-1 that answers INFO and BK_LIST / GET / PUT as firmware editor.c does: a backup, a restore, a damaged file, an
//   older (KARMA) backup with sample slots
// - fm1pkg.js: productOf and logicalImage on build/felucca.fwsc (skipped without a build)
// - fm1ota.js: a full install and an unplug during the write against a simulated FM-1

import { execFileSync } from "node:child_process";
import { existsSync, readFileSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import vm from "node:vm";
import { logicalImage, productOf } from "./fm1pkg.js";
import { Updater, pack7, unpack7, FM1_PORT, safeNames } from "./fm1ota.js";

let failed = 0;
const ok = (cond, what) => { console.log(`${what.padEnd(64)} ${cond ? "ok" : "FAIL"}`); if (!cond) failed++; };
const eq = (a, b) => a.length === b.length && a.every((v, i) => v === b[i]);
const js = (x) => JSON.stringify(x);
const PYTHON = process.env.PYTHON || (process.platform === "win32" ? "python" : "python3");
const py = (code, ...args) => execFileSync(PYTHON, ["-c", code, ...args], { maxBuffer: 1 << 26 });
const HERE = fileURLToPath(new URL(".", import.meta.url));

/* ------------------------------------------------------------ the backup page --- */
const html = readFileSync(join(HERE, "editor.html"), "utf8");
const proto = html.slice(html.indexOf("/*PROTO-BEGIN*/"), html.indexOf("/*PROTO-END*/"));
const E = vm.runInNewContext(proto + `
;({ frame, unframe, parse, req, Link, CMD, pack7, unpack7, crc32, b64enc, b64dec, BK, backupCapture, backupRestore, backupObjects, bkProjFormat })`,
{ setTimeout, clearTimeout, console });

/* a simulated FM-1: its objects (firmware editor.c ED_BK_IDS, project.c *_restore) behind INFO and BK_LIST / GET / PUT */
function mockFM1() {
  const proj = (k) => Uint8Array.from({ length: 3840 }, (_, i) => (i < 4 ? [0x37, 0x4E, 0x55, 0x46][i] : (i * 31 + k) & 0xFF));
  const obj = new Map([[0, proj(0)], [1, Uint8Array.from({ length: 112 }, (_, i) => i)], [2, proj(2)], [3, new Uint8Array(0)],
    [4, new Uint8Array(0)], [5, proj(5)], [6, new Uint8Array(0)], [7, new Uint8Array(0)], [8, new Uint8Array(0)],
    [9, Uint8Array.from({ length: 1464 }, (_, i) => (i * 3) & 0xFF)], [10, Uint8Array.from({ length: 40 }, (_, i) => i & 7)]]);
  let put = null;
  const u35 = (v) => [0, 1, 2, 3, 4].map((k) => Math.floor((v >>> 0) / 2 ** (7 * k)) & 0x7F);
  const r35 = (a, i) => { let v = 0; for (let k = 0; k < 5; k++) v += a[i + k] * 2 ** (7 * k); return v >>> 0; };
  const reply = (cmd, args) => E.frame(cmd, args);
  function handle(d) {
    const f = E.unframe(d);
    if (!f) return null;
    const a = f.a;
    switch (f.cmd) {
      case E.CMD.INFO: { const s = [..."FELUCCA KARMA 3.1.5"].map((c) => c.charCodeAt(0)); return reply(1, [...s, 0, 16, 71, 32, 64, 63, ...Array(16).fill(0), 4, 11]); }
      case E.CMD.BK_LIST: { const out = [0, obj.size]; for (const [id, d2] of obj) out.push(id, ...u35(d2.length), ...u35(E.crc32(d2))); return reply(34, out); }
      case E.CMD.BK_GET: { const id = a[0], off = r35(a, 1), n = a[6] | (a[7] << 7), d2 = obj.get(id);
        const rc = !d2 || off + n > d2.length ? 1 : 0;
        return reply(35, [id, rc, ...u35(off), rc ? 0 : n & 127, rc ? 0 : n >> 7, ...(rc ? [] : E.pack7(d2.subarray(off, off + n)))]); }
      case E.CMD.BK_PUT: { const op = a[0], id = a[1]; let rc = 1;
        if (op === 0 && id <= 10) { put = { id, buf: new Uint8Array(r35(a, 2)), crc: r35(a, 7), pos: 0 }; rc = 0; }
        else if (op === 1 && put && put.id === id) { const b = E.unpack7(a.slice(7)); put.buf.set(b, put.pos); put.pos += b.length; rc = 0; }
        else if (op === 2 && put && put.id === id) { rc = put.pos === put.buf.length && E.crc32(put.buf) === put.crc ? 0 : 2; if (!rc) obj.set(id, put.buf); put = null; }
        return reply(36, [op, id, rc]); }
      default: return null;                         /* (KARMA's editing commands: no answer) */
    }
  }
  return { obj, handle };
}
async function backupPage() {
  const m = mockFM1();
  let link;
  link = new E.Link((d) => { const r = m.handle(d); if (r) setTimeout(() => link.receive(Uint8Array.from(r)), 0); }, { timeout: 200 });
  const rq = (r, o) => link.request(r, o);
  const info = E.parse[E.CMD.INFO](await rq(E.req.info()));
  ok(info.proto >= 6 && /KARMA/.test(info.version), "backup page: INFO (protocol 6 or later)");
  const ec = readFileSync(join(HERE, "../firmware/src/editor.c"), "utf8");
  ok(/ED_BK_IDS\[\] = \{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10\}/.test(ec) && /ED_BK_LIST = 34, ED_BK_GET = 35, ED_BK_PUT = 36/.test(ec),
    "backup page: the object ids and commands == firmware editor.c");
  const A = await E.backupCapture(rq, info);
  ok(A.format === "sloop-backup" && A.objects.map((o) => o.id).join() === "0,1,2,3,4,5,6,7,8,9,10" && A.objects[0].len === 3840,
    "backup page: LIST + GET: every object, as the device has them");
  const before = js([...m.obj].map(([id, d]) => [id, Array.from(d)]));
  m.obj.set(2, new Uint8Array(0));
  m.obj.set(9, new Uint8Array(1464));
  await E.backupRestore(rq, JSON.parse(JSON.stringify(A)));
  ok(js([...m.obj].map(([id, d]) => [id, Array.from(d)])) === before, "backup page: restore -> the same bytes on the device");
  const bad = JSON.parse(JSON.stringify(A));
  bad.objects[2].data = E.b64enc(E.b64dec(bad.objects[2].data).map((x, i) => i === 3 ? x ^ 1 : x));
  let caught = "";
  try { E.backupObjects(bad); } catch (e) { caught = e.code; }
  ok(caught === "bkBad", "backup page: a damaged file is refused before anything is written");
  const old = JSON.parse(JSON.stringify(A));
  old.objects.push({ id: 33, len: 4, crc: E.crc32(Uint8Array.from([1, 2, 3, 4])), data: E.b64enc(Uint8Array.from([1, 2, 3, 4])) });
  const om = E.backupObjects(old);
  ok(!om.has(33) && om.has(0), "backup page: a KARMA backup's sample slot is read and skipped (PHEBUS has none)");
  link.close();
}

/* ------------------------------------------------------- packages: JS == Python --- */
async function packages() {
  const pkg = join(HERE, "../build/felucca.fwsc");
  if (!existsSync(pkg)) {
    console.log("packages: skipped (run ./build.sh first)");
    return;
  }
  const raw = readFileSync(pkg);
  const logical = py(`import sys; raw = open(sys.argv[1], "rb").read()
sys.stdout.buffer.write(b"".join(raw[i * 48:i * 48 + 47] for i in range(20)) + raw[960:])`, pkg);
  ok(eq(logicalImage(raw), logical), "fm1pkg.js logicalImage");
  ok(/^FM-1_9\d\d$/.test(productOf(raw)), "fm1pkg.js productOf");
}

/* ------------------------------------------------- update protocol (fm1ota.js) --- */
const HS = [0xF0, 0x00, 0x32, 0x45, 0x00, 0x00, 0x00, 0x40, 0x7F, 0xF7];
const UPGRADE = [0xF0, 0x22, 0x24, 0x35, 0x7F, 0xF7];

/* an FM-1 on WebMIDI: identity, then "device asks, host answers" reads of the image */
class FakeFM1 {
  constructor(image, { unplugAfter = Infinity, finalIdentity = "FM-1_900", name = "FM-1", identity = "FM-1_015" } = {}) {
    this.image = image; this.unplugAfter = unplugAfter; this.served = 0; this.bad = 0;
    this.finalIdentity = finalIdentity;
    this.access = { inputs: new Map(), outputs: new Map() };
    this.boot(identity, name);
  }
  boot(identity, name) {
    this.identity = identity; this.waiting = null; this.queue = [];
    for (const m of [this.access.inputs, this.access.outputs]) { for (const p of m.values()) p.state = "disconnected"; m.clear(); }
    const id = Math.random().toString(36).slice(2);
    this.input = { id: "i" + id, name, state: "connected", onmidimessage: null, open: async () => {} };
    this.output = { id: "o" + id, name, state: "connected", open: async () => {}, send: (d) => {
      if (this.output.state !== "connected") throw new Error("InvalidStateError");
      setTimeout(() => this.rx(Array.from(d)), 1);
    } };
    this.access.inputs.set(this.input.id, this.input);
    this.access.outputs.set(this.output.id, this.output);
  }
  tx(bytes) { const i = this.input; setTimeout(() => { if (i.state === "connected" && i.onmidimessage) i.onmidimessage({ data: Uint8Array.from(bytes) }); }, 1); }
  rx(d) {
    if (eq(d, HS)) {
      const t = [...new TextEncoder().encode(this.identity)];
      const body = [0, 0x59, 0x11, 0, 0, 0, ...t, ...new Array(28 - t.length).fill(0)];
      this.tx([0xF0, ...pack7(body), 0xF7]);
    } else if (eq(d, UPGRADE)) {
      this.queue = this.identity.startsWith("ota-")
        ? [...Array.from({ length: 6 }, (_, k) => [k * 512, 512]), [0xF0000000, 8]]
        : [[0, 64], [0x40, 160], [0x1000, 512], [0xE0000000, 8]];
      this.next();
    } else if (this.waiting) {
      const u = unpack7(d.slice(1, -1));
      const [addr, len] = this.waiting;
      const got = u.slice(14, 14 + (addr >= 0xE0000000 ? 8 : len));
      const want = addr >= 0xE0000000 ? [...new TextEncoder().encode("success"), 0] : Array.from(this.image.subarray(addr, addr + len));
      if (!eq(got, want)) this.bad++;
      this.waiting = null;
      this.served++;
      if (this.served >= this.unplugAfter) { this.input.state = this.output.state = "disconnected"; return; }
      if (addr === 0xE0000000) setTimeout(() => this.boot("ota-FM-1_900", "Felucca Update"), 300);
      else if (addr === 0xF0000000) setTimeout(() => this.boot(this.finalIdentity, "Felucca"), 300);
      else this.next();
    }
  }
  next() {
    const r = this.queue.shift();
    if (!r) return;
    this.waiting = r;
    const [addr, len] = r;
    const u = [0, 0x59, 0x30, 0, 0, 0, 0, addr & 0xFF, (addr >>> 8) & 0xFF, (addr >>> 16) & 0xFF, (addr >>> 24) & 0xFF, len & 0xFF, len >> 8, 0];
    let s = 0;
    for (let i = 6; i < 14; i++) s += u[i];
    u.push(~s & 0xFF);
    this.tx([0xF0, ...pack7(u), 0xF7]);
  }
}

async function updater() {
  const image = Uint8Array.from({ length: 0x2000 }, (_, i) => (i * 7) & 0xFF);
  const dev = new FakeFM1(image);
  const steps = [];
  const got = await new Updater(dev.access).install(image, "FM-1_900", (k) => steps.push(k));
  ok(got === "FM-1_900" && dev.bad === 0 && steps.includes("write") && steps.at(-1) === "done",
    `fm1ota.js: install: running firmware -> loader -> Felucca (${dev.served} reads)`);

  const rescue = new FakeFM1(image);
  rescue.boot("FM-1_000", "Felucca");
  const recovered = await new Updater(rescue.access).install(image, "FM-1_900");
  ok(recovered === "FM-1_900" && rescue.bad === 0, "fm1ota.js: recovery mode -> loader -> normal firmware");
  const failedBoot = new FakeFM1(image, { finalIdentity: "FM-1_000" });
  const rescueSteps = [];
  const bootError = await new Updater(failedBoot.access).install(image, "FM-1_900", (k) => rescueSteps.push(k)).then(() => null, (e) => e);
  ok(bootError?.code === "mismatch" && !rescueSteps.includes("done"), "fm1ota.js: boot into recovery is not reported as successful installation");

  const dev2 = new FakeFM1(image, { unplugAfter: 3 });
  dev2.boot("ota-FM-1_900", "Felucca Update");
  const t0 = Date.now();
  const done = await new Updater(dev2.access).resume(image);
  ok(done === false && Date.now() - t0 < 6000, "fm1ota.js: unplugged during the write -> stops at once");

  const dev3 = new FakeFM1(image, { unplugAfter: 2 });
  const e = await new Updater(dev3.access).install(image, "FM-1_900").then(() => null, (x) => x);
  ok(e && e.code === "lost", "fm1ota.js: unplugged in step 1 -> error code 'lost'");
  const e2 = await new Updater({ inputs: new Map(), outputs: new Map() }).install(image, "FM-1_900").then(() => null, (x) => x);
  ok(e2 && e2.code === "notfound", "fm1ota.js: no device -> error code 'notfound'");
  {   /* an FM-1 running CLIPPER (USB name "Clipper", SLOOP's sister firmware) installs SLOOP like any FM-1 */
    const cl = new FakeFM1(image, { name: "Clipper", identity: "FM-1_901" });
    const g = await new Updater(cl.access).install(image, "FM-1_900");
    ok(g === "FM-1_900" && cl.bad === 0, "fm1ota.js: an FM-1 under CLIPPER (port 'Clipper') -> found and installed");
    /* other gear is never probed, and "not found" names the MIDI ports there are */
    const other = new FakeFM1(image, { name: "Arturia KeyStep Pro" });
    const e6 = await new Updater(other.access).install(image, "FM-1_900").then(() => null, (x) => x);
    ok(e6 && e6.code === "notfound" && other.served === 0 && /Arturia KeyStep Pro/.test(e6.detail),
       `fm1ota.js: a port of other gear is left alone; 'not found' lists it${e6 ? e6.detail : ""}`);
    {   /* (3.0.4, 2026-11-07: 4 in 10 installs ended "not found") why: no MIDI at all, other gear only, an FM-1 another app
         * holds (open fails), an FM-1 that does not answer; the page shows a checklist for each */
      const port = (name, { busy = false } = {}) => ({ id: name, name, state: "connected", onmidimessage: null,
        open: async () => { if (busy) throw new Error("in use"); }, send: () => {} });
      const acc = (...names) => {
        const m = (o) => new Map(names.map(([n, opt]) => [n, port(n, opt)]));
        return { inputs: m(), outputs: m() };
      };
      const why = async (a) => { const x = await new Updater(a).install(image, "FM-1_900").then(() => null, (y) => y); return x && x.code === "notfound" ? x.reason : `?${x && x.code}`; };
      const r = [await why({ inputs: new Map(), outputs: new Map() }), await why(acc(["Arturia KeyStep Pro"])),
                 await why(acc(["FM-1 KARMA", { busy: true }])), await why(acc(["FM-1 KARMA"]))];
      ok(r.join() === "noports,nofm1,busy,silent", `fm1ota.js: 'not found' says why: no MIDI, other gear, FM-1 in use, FM-1 silent (${r.join()})`);
      const page = readFileSync(new URL("./index_pkg.html", import.meta.url), "utf8");
      ok(["nf_noports", "nf_nofm1", "nf_busy", "nf_silent", "h_cable", "h_direct", "h_on", "h_mac", "h_close", "h_replug", "h_power", "h_again", "found", "h_acable", "h_otg", "h_computer", "nf_noports_a", "nf_nofm1_a"]
           .every((k) => (page.match(new RegExp(`(^|[^A-Za-z_])${k}: "`, "gm")) || []).length === 2),
         "installer: every 'not found' help text in English and Japanese");
      ok(/const HELP_ANDROID = \["h_acable", "h_otg",/.test(page) && /phone \? HELP_ANDROID :/.test(page), "installer: Android gets its own 'not found' list (data cable, OTG)");
      /* (2026-11-08) the counts name the other gear (is a new FM-1 under an unknown name?), never a person's device */
      const e8 = await new Updater(acc(["Arturia KeyStep Pro"], ["Ari's iPhone"])).install(image, "FM-1_900").then(() => null, (y) => y);
      ok(e8 && e8.reason === "nofm1" && e8.names.length === 2, "fm1ota.js: 'not found' lists the MIDI names it met");
      const sn = safeNames(["IAC Driver Bus 1", "Ari\u2019s iPhone", "FM1 MIDI", "Network Session 1", "x"]);
      ok(sn.join("|") === "IAC Driver Bus 1|*|FM1 MIDI", `fm1ota.js: safeNames: three at most, a person's device hidden (${sn.join("|")})`);
      ok(!/stock-go|stock-file|validateStockPackage\(/.test(page) &&
         ["stock1", "stock2", "stock3", "stock4", "stockNote", "stockEditor"].every((k) => (page.match(new RegExp(`(^|[^A-Za-z_])${k}: "`, "gm")) || []).length === 2),
         "installer: back to the official firmware is M-UPGRADE's steps (no button), in English and Japanese");
    }
    ok(["FM-1", "FM-1 KARMA", "Felucca", "Felucca Update", "Clipper", "SLOOP", "MIDIIN2 (Clipper)", "USB-MIDI"].every((n) => FM1_PORT.test(n)) &&
       !["Arturia KeyStep Pro", "loopMIDI Port", "Microsoft GS Wavetable Synth"].some((n) => FM1_PORT.test(n)),
       "fm1ota.js: FM1_PORT takes the FM-1's names (SLOOP, CLIPPER, the loader, the stock firmware), not other gear");
  }
  const stock = new FakeFM1(image);
  stock.boot("ota-FM-1_015", "FM-1 Update");
  const e3 = await new Updater(stock.access).resume(image).then(() => null, (x) => x);
  ok(e3 && e3.code === "foreign" && e3.detail === "ota-FM-1_015" && stock.served === 0, "fm1ota.js: another firmware's loader is never resumed ('foreign')");
  {   /* (3.1.3) M-VAVE's FM1 Open ("FM1 Community"): found, never written over; its own update mode never resumed */
    ok(FM1_PORT.test("FM1 Community"), "fm1ota.js: FM1_PORT finds M-VAVE's FM1 Open (port 'FM1 Community')");
    for (const ident of ["FM-1_8000014", "FM1_COMMUNITY_3", "FM1_OFFICIAL_OPEN"]) {
      const op = new FakeFM1(image, { name: "FM1 Community", identity: ident });
      const eo = await new Updater(op.access).install(image, "FM-1_926").then(() => null, (x) => x);
      ok(eo && eo.code === "open" && eo.detail === ident && op.served === 0, `fm1ota.js: FM1 Open (${ident}): 'open', nothing written`);
    }
    const ol = new FakeFM1(image);
    ol.boot("ota-FM1C_426b7fa5", "FM1 Community");
    const eol = await new Updater(ol.access).resume(image).then(() => null, (x) => x);
    ok(eol && eol.code === "openota" && ol.served === 0, "fm1ota.js: FM1 Open's update mode is never resumed ('openota')");
    const page = readFileSync(new URL("./index_pkg.html", import.meta.url), "utf8");
    ok(["open", "openota", "nf_open", "o_backup", "o_wb", "o_close"].every((k) => (page.match(new RegExp(`(^|[^A-Za-z_])${k}: "`, "gm")) || []).length === 2) &&
       /open: \["o_backup", "o_wb", "o_close", "h_again"\]/.test(page) && /fm1-workbench/.test(page),
       "installer: FM1 Open: the way back to V15 with M-VAVE's FM1 Workbench, in English and Japanese");
  }
  const back = new FakeFM1(image, { finalIdentity: "FM-1_015" });   /* the return to the official V15, interrupted */
  back.boot("ota-FM-1_015", "FM-1 Update");
  const st = [];
  const r4 = await new Updater(back.access).resume(image, (k) => st.push(k), { product: "FM-1_015" });
  ok(r4 === true && back.bad === 0 && st.at(-1) === "done", "fm1ota.js: return to official: its own loader resumed, V15 checked when back");
  const wrong = new FakeFM1(image, { finalIdentity: "FM-1_900" });
  wrong.boot("ota-FM-1_015", "FM-1 Update");
  const e5 = await new Updater(wrong.access).resume(image, null, { product: "FM-1_015" }).then(() => null, (x) => x);
  ok(e5 && e5.code === "mismatch", "fm1ota.js: return to official: another firmware coming back is not 'done'");
  const pk = await import(join(HERE, "fm1pkg.js"));
  const notStock = new Uint8Array(pk.STOCK_V15_SIZE);
  const e6 = await pk.validateStockPackage(notStock).then(() => null, (x) => x);
  ok(e6 && /official FM-1 V15 or V16/.test(e6.message), "fm1pkg.js: only the exact official V15 / V16 are accepted (SHA-256)");
  /* (2026-10-31) the real official files, when they are here: V15 (ref/), V16 (KARMA_V16=path) */
  const fs = await import("node:fs");
  for (const [path, version, product] of [[join(HERE, "../../ref/fm-106/build/FM-1.fwsc"), "V15", "FM-1_015"],
                                           [process.env.KARMA_V16 || "", "V16", "FM-1_016"]]) {
    if (!path || !fs.existsSync(path)) { console.log(`  (skipped: no official ${version} file)`); continue; }
    const raw = new Uint8Array(fs.readFileSync(path));
    const v = await pk.validateStockPackage(raw);
    ok(v.version === version && v.product === product && v.image.length === raw.length - 20, `fm1pkg.js: the official ${version} is accepted as ${version}`);
    raw[raw.length - 100] ^= 1;
    const e7 = await pk.validateStockPackage(raw).then(() => null, (x) => x);
    ok(!!e7, `fm1pkg.js: the official ${version} with one byte changed is refused`);
  }
}

await backupPage();
await packages();
await updater();
console.log(failed ? `WEB TESTS FAILED (${failed})` : "web tests passed");
process.exit(failed ? 1 : 0);
