import assert from "node:assert/strict";
import test from "node:test";

import { runFixture, validateFixture } from "../helpers/runner.ts";

test("validate returns ok for a complete fixture", () => {
  assert.equal(validateFixture("balanced_quote.json"), "ok");
});

test("run emits stable top-level report fields", () => {
  const report = runFixture("balanced_quote.json", "run", ["--events"]);

  assert.equal(report.scenario, "balanced_quote");
  assert.equal(report.status, "ok");
  assert.equal(report.clock.window, 30000001);
  assert.equal(report.quotes.length, 1);
  assert.equal(report.settlements.length, 1);
  assert.equal(report.events.settlement.length, 3);
});
