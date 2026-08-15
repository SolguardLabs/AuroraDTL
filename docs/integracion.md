# Integración

## Entrada

Los importes se codifican como cadenas de enteros. Los precios son cadenas
decimales. Las secuencias, timestamps, ventanas y bps son enteros JSON.

Campos de una orden:

| Campo          | Tipo    | Regla                                  |
| -------------- | ------- | -------------------------------------- |
| `id`           | string  | identificador estable                  |
| `account`      | string  | cuenta existente                       |
| `route`        | string  | ruta registrada                        |
| `amountIn`     | string  | entero positivo en precisión de origen |
| `minOut`       | string  | entero en precisión de destino         |
| `toleranceBps` | number  | 0 a 10.000                             |
| `window`       | number  | ventana autorizada                     |
| `settle`       | boolean | `false` para quote-only                |
| `memo`         | string  | referencia operativa opcional          |

## Salida

`quotes` conserva `grossOut`, `netOut`, `indexOut`, tolerancia y legs. Los
`settlements` añaden débito, crédito, fee, base de liquidación y estado. Los
balances e importes permanecen como strings.

`stress` incluye nocional base, nocional afectado, pérdida, cobertura, HHI,
severidad y detalle por activo. `checkpoint` incluye secuencia, ventana, enlace,
digest, filas y eventos.

## Errores

La CLI devuelve código distinto de cero para entrada mal formada, comando
desconocido o incumplimiento estructural. Una orden económicamente rechazada se
devuelve dentro del JSON con `accepted: false` o `status: rejected`; no es un
fallo del proceso.

## Transporte

`AuroraClient` acepta una interfaz mínima:

```typescript
interface AuroraTransport {
  execute(command, fixture, options): Promise<string>;
}
```

El adaptador puede usar un proceso local, job aislado o servicio remoto. Debe
limitar comandos y rutas, imponer timeout, capturar stdout/stderr por separado y
verificar el tamaño máximo del resultado antes de parsear JSON.

## Idempotencia

`validate` y `quote` son lecturas. `settle` es determinista sobre un escenario
completo, pero un orquestador persistente debe impedir reutilizar el mismo `id`
en una ventana ya confirmada.
