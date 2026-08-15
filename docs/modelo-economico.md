# Modelo económico

## Dominios de importe

Cada activo tiene una precisión `d`. Un entero `A` representa `A / 10^d`
unidades. El motor conserva además una representación indexada a 18 decimales
para análisis homogéneo. Ambas representaciones deben viajar con su dominio.

Para convertir origen `s` a destino `d`:

```text
U_s = A_s / 10^d_s
U_d = U_s · P_s / P_d
A_d = round(U_d · 10^d_d)
I_d = round(U_d · 10^18)
```

Ejemplo WETH18 a USDC6:

```text
A_s = 1,000,000,000,000
U_s = 0.000001 WETH
P_s = 3,000 USD
P_d = 1 USD
U_d = 0.003 USDC
A_d = 3,000 unidades USDC6
I_d = 3,000,000,000,000,000 unidades indexadas
```

## Fees y tolerancia

La comisión total agrega base, ruta, ventana y override del activo:

```text
fee_bps = base_bps + route_bps + window_bps + asset_bps
fee     = floor(gross · fee_bps / 10,000)
net     = gross - fee
floor   = net - floor(net · tolerance_bps / 10,000)
```

Una orden se acepta cuando `minOut <= net` y, si `minOut` es distinto de cero,
`minOut >= floor`. La tolerancia efectiva es el mínimo entre la solicitada por
cliente y la política más estricta aplicable.

## Stress de cartera

Para cada activo `i`, el nocional base es:

```text
N_i = balance_i / 10^d_i · price_i
shock_i = min(10,000, marketShock + confidence_i · multiplier / 10,000)
N'_i = N_i · (1 - shock_i) · (1 - accessibilityHaircut)
loss = ΣN_i - ΣN'_i
uncovered = max(0, loss - reserve)
```

La concentración usa Herfindahl-Hirschman:

```text
share_i = N_i / ΣN_i
HHI_bps = 10,000 · Σ share_i²
```

Una cartera repartida entre cuatro activos iguales obtiene `2,500 bps`; una
cartera concentrada en un único activo obtiene `10,000 bps`.

## Clasificación operativa

- `low`: pérdida inferior a 7,5% y concentración limitada;
- `moderate`: pérdida desde 7,5% o HHI desde 2.500 bps;
- `high`: pérdida desde 15% con déficit o HHI desde 5.000 bps;
- `critical`: pérdida desde 25% con déficit de reservas.

Los umbrales son señales de operación, no garantías de solvencia. Deben
calibrarse por mercado y revisarse junto con liquidez real, correlación y
capacidad de reposición.
