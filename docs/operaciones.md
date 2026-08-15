# Operaciones

## Preapertura de ventana

1. Verificar commit y checksum del binario.
2. Confirmar que la ventana del escenario coincide con la ventana autorizada.
3. Comprobar secuencias y antigüedad de todos los precios.
4. Ejecutar `validate` sobre el fixture final.
5. Ejecutar `quote` y revisar aceptación, fees, stress y concentración.
6. Confirmar capacidad disponible por ruta y activo.
7. Archivar el checkpoint previo firmado.

## Ejecución

```bash
out/auroradtl validate scenario.json
out/auroradtl quote scenario.json --json > quote.json
out/auroradtl settle scenario.json --json --events > settlement.json
```

No reutilices el resultado de `quote` si cambió el precio, la ventana, la ruta,
el catálogo o cualquier balance de origen. El servicio que coordina archivos
debe usar rutas absolutas permitidas y escritura atómica.

## Señales mínimas

| Señal                |         Umbral de atención | Acción                        |
| -------------------- | -------------------------: | ----------------------------- |
| Precio rechazado     |                          1 | detener rutas del activo      |
| Orden rechazada      | crecimiento frente a media | revisar tolerancia y ventana  |
| HHI                  |                ≥ 5.000 bps | limitar exposición dominante  |
| Cobertura de reserva |                < 8.000 bps | reponer o reducir capacidad   |
| Stress               |        `high` o `critical` | pausa de nuevas liquidaciones |
| Conciliación         |       cualquier diferencia | preservar estado y escalar    |
| Cadena de checkpoint |          enlace incorrecto | rechazar publicación          |

## Cierre de ventana

Conserva escenario, salida completa, eventos, balances, informe de conciliación,
stress y digest. Dos operadores independientes comparan el digest canónico. El
servicio externo recoge firmas y publica el manifiesto de ventana.

## Recuperación

La recuperación parte del último checkpoint firmado, no de un archivo mutable.
Reproduce precios y órdenes en una copia, compara balances por cuenta/activo y
avanza ventana por ventana. Si falta un evento, no se sintetiza: se documenta
la discontinuidad y se mantiene la pausa.
