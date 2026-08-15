import assert from "node:assert/strict";
import test from "node:test";

import { runFixture } from "../helpers/runner.ts";

test("run emits a deterministic operational checkpoint", () => {
  const first = runFixture("balanced_quote.json", "run", ["--events"]);
  const second = runFixture("balanced_quote.json", "run", ["--events"]);

  assert.equal(first.checkpoint.sequence, 1);
  assert.equal(first.checkpoint.window, 30000001);
  assert.equal(first.checkpoint.digest.length, 64);
  assert.equal(first.checkpoint.digest, second.checkpoint.digest);
  assert.equal(first.checkpoint.eventCount, first.events.settlement.length);
  assert.ok(first.checkpoint.balanceRows > 0);
});

test("report includes portfolio downside and concentration", () => {
  const report = runFixture("multi_asset_route.json", "run");

  assert.equal(report.stress.scenario, "correlated-downside");
  assert.ok(Number(report.stress.baselineNotional) > 0);
  assert.ok(Number(report.stress.shockedNotional) < Number(report.stress.baselineNotional));
  assert.ok(report.stress.lossBps > 0);
  assert.ok(report.stress.concentrationHhiBps > 0);
  assert.ok(report.stress.assets.length >= 2);
});

test("changed settlement state changes the operational digest", () => {
  const quote = runFixture("balanced_quote.json", "quote");
  const settled = runFixture("balanced_quote.json", "run");

  assert.notEqual(quote.checkpoint.digest, settled.checkpoint.digest);
  assert.equal(quote.checkpoint.eventCount, 0);
  assert.ok(settled.checkpoint.eventCount > 0);
});
