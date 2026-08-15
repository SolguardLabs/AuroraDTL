export const BPS = 10_000n;
export const INDEX_DECIMALS = 18;

export type AmountLike = bigint | number | string;

export interface AuroraTransport {
  execute(
    command: "validate" | "quote" | "run" | "settle",
    fixture: string,
    options?: string[],
  ): Promise<string>;
}

export interface QuoteView {
  id: string;
  source: string;
  destination: string;
  grossOut: string;
  netOut: string;
  minOut: string;
  indexOut: string;
  accepted: boolean;
  reason: string;
}

export interface SettlementView {
  id: string;
  status: string;
  settlementGross: string;
  destinationCredit: string;
  destinationFee: string;
  quote: QuoteView;
}

export interface OperationalReport {
  scenario: string;
  status: string;
  quotes: QuoteView[];
  settlements: SettlementView[];
  balances: Array<{ account: string; asset: string; amount: string }>;
  stress: {
    baselineNotional: string;
    shockedNotional: string;
    lossNotional: string;
    reserveCoverageBps: number;
    concentrationHhiBps: number;
    severity: string;
  };
  checkpoint: {
    sequence: number;
    window: number;
    previousDigest: string;
    digest: string;
  };
}

export interface PortfolioPosition {
  asset: string;
  amountRaw: bigint;
  decimals: number;
  priceE8: bigint;
  shockBps: number;
  accessibilityHaircutBps?: number;
}

export interface PortfolioStressResult {
  baselineE8: bigint;
  shockedE8: bigint;
  lossE8: bigint;
  uncoveredLossE8: bigint;
  reserveCoverageBps: number;
  largestAssetShareBps: number;
  concentrationHhiBps: number;
  byAsset: Array<{ asset: string; baselineE8: bigint; shockedE8: bigint; shareBps: number }>;
}

export interface WindowPlan {
  accepted: boolean;
  window: bigint;
  requestedRaw: bigint;
  remainingRaw: bigint;
  projectedUtilizationBps: number;
  reason: "accepted" | "window-mismatch" | "capacity-exceeded";
}

function assertDecimals(decimals: number): void {
  if (!Number.isInteger(decimals) || decimals < 0 || decimals > 18) {
    throw new RangeError("decimals must be an integer between 0 and 18");
  }
}

function assertBps(value: number, field: string): void {
  if (!Number.isInteger(value) || value < 0 || value > 10_000) {
    throw new RangeError(`${field} must be an integer between 0 and 10000`);
  }
}

function pow10(decimals: number): bigint {
  assertDecimals(decimals);
  return 10n ** BigInt(decimals);
}

export function asBigInt(value: AmountLike, field = "amount"): bigint {
  if (typeof value === "bigint") {
    if (value < 0n) throw new RangeError(`${field} must be non-negative`);
    return value;
  }
  if (typeof value === "number") {
    if (!Number.isSafeInteger(value) || value < 0)
      throw new RangeError(`${field} must be a non-negative safe integer`);
    return BigInt(value);
  }
  if (!/^(0|[1-9][0-9]*)$/.test(value))
    throw new TypeError(`${field} must be an unsigned integer string`);
  return BigInt(value);
}

export function parseUnits(value: string, decimals: number): bigint {
  assertDecimals(decimals);
  if (!/^(0|[1-9][0-9]*)(\.[0-9]+)?$/.test(value))
    throw new TypeError("value must be an unsigned decimal string");
  const [whole, fraction = ""] = value.split(".");
  if (fraction.length > decimals) throw new RangeError("value exceeds asset precision");
  return (
    BigInt(whole) * pow10(decimals) +
    BigInt((fraction + "0".repeat(decimals)).slice(0, decimals) || "0")
  );
}

export function formatUnits(value: AmountLike, decimals: number): string {
  const raw = asBigInt(value);
  const scale = pow10(decimals);
  const whole = raw / scale;
  const fraction = (raw % scale).toString().padStart(decimals, "0").replace(/0+$/, "");
  return fraction ? `${whole}.${fraction}` : whole.toString();
}

export function normalizeTo18(value: AmountLike, decimals: number): bigint {
  const raw = asBigInt(value);
  assertDecimals(decimals);
  return raw * pow10(INDEX_DECIMALS - decimals);
}

export function denormalizeFrom18(value: AmountLike, decimals: number): bigint {
  const normalized = asBigInt(value);
  assertDecimals(decimals);
  return normalized / pow10(INDEX_DECIMALS - decimals);
}

export function quoteByPrice(
  sourceRaw: AmountLike,
  sourceDecimals: number,
  destinationDecimals: number,
  sourcePriceE8: AmountLike,
  destinationPriceE8: AmountLike,
): bigint {
  const source = asBigInt(sourceRaw, "sourceRaw");
  const sourcePrice = asBigInt(sourcePriceE8, "sourcePriceE8");
  const destinationPrice = asBigInt(destinationPriceE8, "destinationPriceE8");
  if (destinationPrice === 0n) throw new RangeError("destinationPriceE8 must be positive");
  return (
    (source * sourcePrice * pow10(destinationDecimals)) / (destinationPrice * pow10(sourceDecimals))
  );
}

export function applyFee(value: AmountLike, feeBps: number): { fee: bigint; net: bigint } {
  assertBps(feeBps, "feeBps");
  const gross = asBigInt(value);
  const fee = (gross * BigInt(feeBps)) / BPS;
  return { fee, net: gross - fee };
}

export function toleranceFloor(value: AmountLike, toleranceBps: number): bigint {
  assertBps(toleranceBps, "toleranceBps");
  const raw = asBigInt(value);
  return raw - (raw * BigInt(toleranceBps)) / BPS;
}

export function deviationBps(previous: AmountLike, next: AmountLike): number {
  const left = asBigInt(previous, "previous");
  const right = asBigInt(next, "next");
  const high = left > right ? left : right;
  if (high === 0n) return 0;
  const low = left < right ? left : right;
  return Number(((high - low) * BPS + high / 2n) / high);
}

export function stressPortfolio(
  positions: PortfolioPosition[],
  reserveE8: AmountLike,
): PortfolioStressResult {
  const reserve = asBigInt(reserveE8, "reserveE8");
  const rows = positions.map((position) => {
    assertDecimals(position.decimals);
    assertBps(position.shockBps, "shockBps");
    const accessibility = position.accessibilityHaircutBps ?? 0;
    assertBps(accessibility, "accessibilityHaircutBps");
    const baselineE8 = (position.amountRaw * position.priceE8) / pow10(position.decimals);
    const afterPrice = (baselineE8 * BigInt(10_000 - position.shockBps)) / BPS;
    const shockedE8 = (afterPrice * BigInt(10_000 - accessibility)) / BPS;
    return { asset: position.asset, baselineE8, shockedE8, shareBps: 0 };
  });
  const baselineE8 = rows.reduce((sum, row) => sum + row.baselineE8, 0n);
  const shockedE8 = rows.reduce((sum, row) => sum + row.shockedE8, 0n);
  const lossE8 = baselineE8 > shockedE8 ? baselineE8 - shockedE8 : 0n;
  const uncoveredLossE8 = lossE8 > reserve ? lossE8 - reserve : 0n;
  let largestAssetShareBps = 0;
  let hhiNumerator = 0n;
  for (const row of rows) {
    row.shareBps = baselineE8 === 0n ? 0 : Number((row.baselineE8 * BPS) / baselineE8);
    largestAssetShareBps = Math.max(largestAssetShareBps, row.shareBps);
    hhiNumerator += BigInt(row.shareBps) * BigInt(row.shareBps);
  }
  const reserveCoverageBps =
    lossE8 === 0n
      ? 10_000
      : Number((reserve * BPS > lossE8 * BPS ? lossE8 * BPS : reserve * BPS) / lossE8);
  return {
    baselineE8,
    shockedE8,
    lossE8,
    uncoveredLossE8,
    reserveCoverageBps,
    largestAssetShareBps,
    concentrationHhiBps: Number(hhiNumerator / BPS),
    byAsset: rows,
  };
}

export function planWindow(
  expectedWindow: AmountLike,
  orderWindow: AmountLike,
  usedRaw: AmountLike,
  capacityRaw: AmountLike,
  requestedRaw: AmountLike,
): WindowPlan {
  const expected = asBigInt(expectedWindow, "expectedWindow");
  const order = asBigInt(orderWindow, "orderWindow");
  const used = asBigInt(usedRaw, "usedRaw");
  const capacity = asBigInt(capacityRaw, "capacityRaw");
  const requested = asBigInt(requestedRaw, "requestedRaw");
  const remaining = capacity > used ? capacity - used : 0n;
  const projected = used + requested;
  const projectedUtilizationBps =
    capacity === 0n
      ? 10_000
      : Number((projected * BPS > capacity * BPS ? capacity * BPS : projected * BPS) / capacity);
  if (order !== expected) {
    return {
      accepted: false,
      window: expected,
      requestedRaw: requested,
      remainingRaw: remaining,
      projectedUtilizationBps,
      reason: "window-mismatch",
    };
  }
  if (requested > remaining) {
    return {
      accepted: false,
      window: expected,
      requestedRaw: requested,
      remainingRaw: remaining,
      projectedUtilizationBps,
      reason: "capacity-exceeded",
    };
  }
  return {
    accepted: true,
    window: expected,
    requestedRaw: requested,
    remainingRaw: remaining - requested,
    projectedUtilizationBps,
    reason: "accepted",
  };
}

export class AuroraClient {
  private readonly transport: AuroraTransport;

  constructor(transport: AuroraTransport) {
    this.transport = transport;
  }

  async validate(fixture: string): Promise<boolean> {
    return (await this.transport.execute("validate", fixture)).trim() === "ok";
  }

  async quote(fixture: string): Promise<OperationalReport> {
    return JSON.parse(
      await this.transport.execute("quote", fixture, ["--json"]),
    ) as OperationalReport;
  }

  async settle(fixture: string, includeEvents = false): Promise<OperationalReport> {
    const options = includeEvents ? ["--json", "--events"] : ["--json"];
    return JSON.parse(
      await this.transport.execute("settle", fixture, options),
    ) as OperationalReport;
  }
}
