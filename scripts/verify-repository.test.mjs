import assert from "node:assert/strict";
import { readFileSync, readdirSync, statSync } from "node:fs";
import { dirname, extname, join, relative, resolve } from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");

function filesBelow(directory) {
  const result = [];
  for (const entry of readdirSync(directory)) {
    if ([".git", "node_modules", "out", "build", "coverage", "private"].includes(entry)) continue;
    const path = join(directory, entry);
    if (statSync(path).isDirectory()) result.push(...filesBelow(path));
    else result.push(path);
  }
  return result;
}

test("documentation set and release metadata are complete", () => {
  const expected = [
    "arquitectura.md",
    "despliegue.md",
    "gobierno.md",
    "integracion.md",
    "modelo-economico.md",
    "modelo-seguridad.md",
    "operaciones.md",
  ];
  assert.deepEqual(readdirSync(join(root, "docs")).sort(), expected);
  const packageJson = JSON.parse(readFileSync(join(root, "package.json"), "utf8"));
  assert.equal(packageJson.version, "1.0.0");
  const workflow = readFileSync(
    join(root, ".github", "workflows", "release-integrity.yml"),
    "utf8",
  );
  assert.match(workflow, /origin\/production/);
  assert.match(workflow, /cat-file -t/);
});

test("banner keeps the shared canvas and valid PNG signature", () => {
  const banner = readFileSync(join(root, "assets", "banner.png"));
  assert.deepEqual([...banner.subarray(0, 8)], [137, 80, 78, 71, 13, 10, 26, 10]);
  assert.equal(banner.readUInt32BE(16), 1672);
  assert.equal(banner.readUInt32BE(20), 941);
});

test("public text contains no internal exercise terminology", () => {
  const blocked = [
    "c" + "tf",
    "vulnera" + "ble",
    "vulnera" + "bilidad",
    "explo" + "it",
    "laborato" + "rio",
  ];
  const textExtensions = new Set([
    "",
    ".cpp",
    ".hpp",
    ".json",
    ".md",
    ".mjs",
    ".ps1",
    ".sh",
    ".ts",
    ".yml",
    ".yaml",
  ]);
  const hits = [];
  for (const path of filesBelow(root)) {
    if (!textExtensions.has(extname(path).toLowerCase())) continue;
    const content = readFileSync(path, "utf8").toLowerCase();
    for (const term of blocked) {
      if (content.includes(term)) hits.push(`${relative(root, path)}:${term}`);
    }
  }
  assert.deepEqual(hits, []);
});
