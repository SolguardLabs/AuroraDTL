import { existsSync, mkdirSync, rmSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { spawnSync } from "node:child_process";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const outDir = join(root, "out");
const exeName = process.platform === "win32" ? "auroradtl.exe" : "auroradtl";
const output = join(outDir, exeName);
const args = new Set(process.argv.slice(2));
const warnings = args.has("--warnings");
const clean = args.has("--clean");

const sources = [
  "src/common.cpp",
  "src/json.cpp",
  "src/amount.cpp",
  "src/asset.cpp",
  "src/oracle.cpp",
  "src/fees.cpp",
  "src/route.cpp",
  "src/ledger.cpp",
  "src/ids.cpp",
  "src/journal.cpp",
  "src/window.cpp",
  "src/quote.cpp",
  "src/settlement.cpp",
  "src/reconcile.cpp",
  "src/metrics.cpp",
  "src/risk.cpp",
  "src/audit.cpp",
  "src/model.cpp",
  "src/report.cpp",
  "src/main.cpp",
].map((file) => join(root, file));

function tryRun(command, args, options = {}) {
  return spawnSync(command, args, {
    cwd: root,
    encoding: "utf8",
    stdio: options.stdio ?? "pipe",
    shell: false,
  });
}

function commandExists(command) {
  if (process.platform === "win32") {
    return tryRun("where.exe", [command]).status === 0;
  }
  return spawnSync("sh", ["-c", `command -v "${command.replaceAll('"', '\\"')}"`], {
    cwd: root,
    encoding: "utf8",
    stdio: "pipe",
  }).status === 0;
}

function findMsvcVcvars() {
  const candidates = [
    "C:\\Program Files\\Microsoft Visual Studio\\18\\Insiders\\VC\\Auxiliary\\Build\\vcvars64.bat",
    "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat",
    "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Auxiliary\\Build\\vcvars64.bat",
    "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Auxiliary\\Build\\vcvars64.bat",
    "C:\\Program Files\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat",
  ];
  return candidates.filter((path) => existsSync(path));
}

function compilerCandidates() {
  const explicit = process.env.CXX ? [process.env.CXX] : [];
  const native = process.platform === "win32" ? ["clang++", "g++", "c++", "cl"] : ["c++", "g++", "clang++"];
  const discovered = process.platform === "win32" ? findMsvcVcvars().map((path) => `vcvars:${path}`) : [];
  return [...explicit, ...native, ...discovered].filter(
    (value, index, array) => value && array.indexOf(value) === index,
  );
}

function cmdQuote(value) {
  return `"${String(value).replaceAll('"', '\\"')}"`;
}

function objectDirForMsvc() {
  return outDir.replaceAll("\\", "/") + "/";
}

function buildWithMsvc(command) {
  const flags = [
    "/std:c++17",
    "/EHsc",
    "/O2",
    "/D_CRT_SECURE_NO_WARNINGS",
    "/Fo" + objectDirForMsvc(),
    "/Fe:" + output,
    ...sources,
  ];
  if (warnings) {
    flags.unshift("/WX", "/W4");
  }
  return tryRun(command, flags, { stdio: "inherit" });
}

function buildWithMsvcVcvars(vcvarsPath) {
  const flags = [
    "/std:c++17",
    "/EHsc",
    "/O2",
    "/D_CRT_SECURE_NO_WARNINGS",
    "/Fo" + objectDirForMsvc(),
    "/Fe:" + output,
    ...sources,
  ];
  if (warnings) {
    flags.unshift("/WX", "/W4");
  }
  const command = `${cmdQuote(vcvarsPath)} >nul && cl ${flags.map(cmdQuote).join(" ")}`;
  return spawnSync(command, {
    cwd: root,
    encoding: "utf8",
    shell: true,
    stdio: "inherit",
  });
}

function buildWithUnixCompiler(command) {
  const flags = ["-std=c++17", "-O2", "-I", join(root, "src"), "-o", output, ...sources];
  if (warnings) {
    flags.splice(2, 0, "-Werror", "-Wall", "-Wextra", "-Wpedantic");
  }
  return tryRun(command, flags, { stdio: "inherit" });
}

if (clean) {
  rmSync(outDir, { recursive: true, force: true });
}
mkdirSync(outDir, { recursive: true });

let attempted = [];
let lastStatus = 1;
for (const compiler of compilerCandidates()) {
  if (compiler.startsWith("vcvars:")) {
    attempted.push("msvc-vcvars");
    const result = buildWithMsvcVcvars(compiler.slice("vcvars:".length));
    lastStatus = result.status ?? 1;
    if (lastStatus === 0) {
      console.log(output);
      process.exit(0);
    }
    continue;
  }
  if (!commandExists(compiler)) {
    continue;
  }
  attempted.push(compiler);
  const result = compiler === "cl" ? buildWithMsvc(compiler) : buildWithUnixCompiler(compiler);
  lastStatus = result.status ?? 1;
  if (lastStatus === 0) {
    console.log(output);
    process.exit(0);
  }
}

if (attempted.length === 0) {
  console.error("No C++ compiler found. Install g++, clang++, c++, or run from a Visual Studio Developer Prompt.");
} else {
  console.error(`Compilation failed with: ${attempted.join(", ")}`);
}
process.exit(lastStatus || 1);
