# AuroraDTL

![banner](./assets/banner.png)

AuroraDTL es un motor C++ de pricing y settlement DTL para rutas internas entre
assets. Modela oraculos de precio, bandas de tolerancia, fees por ruta, ventanas
de settlement y ejecucion multi-asset desde fixtures JSON reproducibles.

El binario no depende de servicios externos. Los tests TypeScript tratan la CLI
como contrato publico y validan quotes, actualizacion de precios, desviaciones,
fees, balances y rutas con varios hops.

## Componentes

- `src/asset.*`: catalogo de assets, decimales, estados y limites de trade.
- `src/oracle.*`: oraculos internos, secuencias, confianza y bandas de cambio.
- `src/route.*`: rutas entre assets y simulacion de legs.
- `src/quote.*`: calculo de quotes, tolerancia y desglose de fees.
- `src/settlement.*`: ejecucion por ventanas y actualizacion de ledger.
- `src/ledger.*`: balances por cuenta y eventos contables.
- `src/window.*`: utilidades de ventanas y admision por capacidad.
- `src/risk.*`: metricas de riesgo de rutas y exposicion.
- `src/reconcile.*`: agregados de settlement y ledger.
- `src/report.*`: salida JSON estable para integraciones y tests.

## Requisitos

- Node.js 24 o superior.
- Un compilador C++17 disponible como `cl`, `g++`, `clang++` o `c++`.

En Windows, el script de build localiza `vcvars64.bat` de Visual Studio cuando
`cl` no esta directamente en `PATH`.

## Uso

Compilar:

```bash
node scripts/build.mjs
```

Validar un fixture:

```bash
out/auroradtl validate tests/fixtures/balanced_quote.json
```

Emitir quotes:

```bash
out/auroradtl quote tests/fixtures/precision_quote.json --json
```

Ejecutar settlement:

```bash
out/auroradtl run tests/fixtures/balanced_quote.json --json --events
```

## Tests

```bash
npm test
```

El script compila el binario y ejecuta:

```bash
node --test --experimental-strip-types "tests/node/*.test.ts"
```

## Fixtures

Los escenarios JSON definen:

- assets con simbolo, decimales, tolerancia y limites operativos;
- precios internos con confianza, timestamp y secuencia;
- actualizaciones opcionales de oraculo;
- rutas con hops, fee adicional y ventana de settlement;
- cuentas iniciales;
- ordenes exact-in con `amountIn`, `minOut`, tolerancia cliente y ventana.

Los importes se expresan como enteros en la precision nativa de cada asset.

## Estado Del Proyecto

AuroraDTL esta preparado como repositorio autocontenido de auditoria. La salida
JSON de la CLI es estable para pruebas de regresion y herramientas externas.
