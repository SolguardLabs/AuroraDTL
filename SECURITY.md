# Security Policy

## Modelo De Seguridad

AuroraDTL asume que los fixtures representan entradas firmadas o aprobadas por
un plano de control externo. El motor valida catalogo de assets, secuencias de
oraculo, bandas de desviacion, rutas habilitadas, tolerancia de quote y balances
antes de liquidar.

## Invariantes Esperadas

- Cada asset usado por rutas, cuentas y precios debe existir en el catalogo.
- Las actualizaciones de oraculo deben ser monotonicamente crecientes por
  secuencia.
- Una actualizacion que supere la banda de desviacion del asset no reemplaza el
  ultimo precio aceptado.
- Las rutas deben contener al menos dos assets y no pueden repetir assets
  adyacentes.
- Las ordenes solo se liquidan si el quote cumple `minOut` y la tolerancia
  efectiva.
- El ledger no permite debitos por encima del balance disponible.
- Los fees se contabilizan en la cuenta configurada como receptor de fees.

## Validaciones Automatizadas

La suite TypeScript cubre:

- contrato basico de CLI;
- quotes single-hop;
- quotes entre assets con distinta precision;
- actualizaciones de precio aceptadas;
- rechazo de desviaciones fuera de banda;
- composicion de fees;
- rutas multi-asset;
- balances posteriores al settlement.

## Gestion De Dependencias

El motor C++ no usa dependencias de runtime externas. Node se utiliza para build
y tests. Dependabot esta configurado para `npm` y GitHub Actions.

## Alcance De Revision

Se consideran dentro de alcance:

- `src/`;
- fixtures bajo `tests/fixtures/`;
- scripts de build y CI;
- tests TypeScript.

Quedan fuera de alcance artefactos locales generados en `out/`, `build/`,
`node_modules/` y variables de entorno locales.

## Reporte Interno

Un reporte debe incluir:

- fixture minimo de reproduccion;
- comando exacto de CLI;
- salida JSON relevante;
- impacto economico esperado;
- propuesta de test de regresion;
- mitigacion directa sobre el componente afectado.
