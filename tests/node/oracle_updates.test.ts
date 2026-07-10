import assert from "node:assert/strict";
import test from "node:test";

import { byId, runFixture } from "../helpers/runner.ts";

test("accepted price update changes the executed quote", () => {
  const report = runFixture("price_update.json");
  const decision = report.oracleUpdates[0];
  const quote = byId(report.quotes, "ord-update-1");

  assert.equal(decision.accepted, true);
  assert.equal(decision.reason, "accepted");
  assert.equal(decision.deviationBps, 99);
  assert.equal(quote.grossOut, "1010000000000000");
  assert.equal(quote.accepted, true);
});

test("deviation guard keeps the last accepted oracle value", () => {
  const report = runFixture("deviation_guard.json");
  const decision = report.oracleUpdates[0];
  const quote = byId(report.quotes, "ord-guard-1");

  assert.equal(decision.accepted, false);
  assert.equal(decision.reason, "deviation-limit");
  assert.equal(decision.deviationBps, 2000);
  assert.equal(quote.grossOut, "1000000000000000");
  assert.equal(quote.accepted, true);
});
