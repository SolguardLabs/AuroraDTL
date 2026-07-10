import assert from "node:assert/strict";
import test from "node:test";

import { balance, byId, runFixture } from "../helpers/runner.ts";

test("settlement debits source and credits destination account", () => {
  const report = runFixture("balanced_quote.json");
  const settlement = byId(report.settlements, "ord-balanced-1");

  assert.equal(settlement.status, "settled");
  assert.equal(settlement.sourceDebit, "1000000000000000");
  assert.equal(settlement.destinationCredit, "998000000000000");
  assert.equal(balance(report, "alice", "USD18"), "1000000000000000");
  assert.equal(balance(report, "alice", "EUR18"), "998000000000000");
});
