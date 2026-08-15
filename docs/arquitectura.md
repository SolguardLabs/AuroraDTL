# Arquitectura

AuroraDTL divide la ejecución en catálogo, valoración, decisión, contabilidad y
observabilidad. Las estructuras usan enteros de 64 bits para importes y
`long double` únicamente para precios, nocionales y escenarios de stress.

## Flujo principal

```mermaid
sequenceDiagram
    participant C as Cliente
    participant M as Model loader
    participant O as OracleBook
    participant Q as QuoteEngine
    participant S as SettlementEngine
    participant L as Ledger
    participant P as Plano operativo
    C->>M: escenario JSON
    M->>M: valida assets, cuentas, rutas y órdenes
    M->>O: aplica actualizaciones secuenciales
    M->>Q: QuoteRequest
    Q->>O: precio y frescura por hop
    Q-->>S: QuoteResult aceptado
    S->>L: débito de origen
    S->>L: crédito de destino y fee
    L-->>P: balances y eventos
    P-->>C: stress, checkpoint y JSON
```

## Límites de responsabilidad

`AssetRegistry` define precisión, estado, tolerancia, mínimo, máximo y permiso
de liquidación. `OracleBook` no obtiene precios: valida puntos entregados por el
plano de control. `RouteBook` simula cada hop. `QuoteEngine` aplica la política
económica. `SettlementEngine` ejecuta el movimiento. `Ledger` conserva balances
y eventos sin conocer precios.

El plano operativo lee el estado resultante. `Reconciler` cruza movimientos y
resultados, `PortfolioStressEngine` valora exposición y `CheckpointBuilder`
produce una huella estable. Esa huella no sustituye una firma digital; permite
que un servicio externo firme exactamente el mismo estado canónico.

## Datos deterministas

Las entradas se ordenan cuando el orden no tiene significado económico. Los
eventos conservan el orden de ejecución. El JSON serializa importes como cadenas
para evitar pérdida de precisión en consumidores JavaScript.

## Dependencias

El binario no enlaza librerías externas. Node.js orquesta builds, tests y el SDK.
Esta separación reduce superficie de suministro y permite compilar en MSVC,
GCC y Clang con warnings tratados como errores.
