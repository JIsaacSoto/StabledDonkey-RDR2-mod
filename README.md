# StabledDonkey

**Burros y mulas como caballos en Red Dead Redemption 2 (Story Mode): vínculo, cuidados, estadísticas y establo.**

StabledDonkey hace que el burro y la mula funcionen como una raza de caballo más, usando los propios sistemas del juego. No reemplaza sus modelos por caballos ni modifica archivos del juego: todos los cambios se aplican en memoria al arrancar.

## Qué hace (v0.3.0)

### Burro

- **Vínculo natural:** montar, alimentar, cepillar y acariciar dan XP de vínculo, con el toast y las subidas de nivel del juego.
- **Ficha y estadísticas:** "Mostrar información" y `Personaje → Caballo` muestran la ficha del burro con sus datos.
- **Establo:** puedes guardar el burro, elegirlo como montura principal y recuperarlo. Vuelve el burro real, con su nombre y su aspecto, también después de cerrar el juego.
- **Cuidados a pie:** al fijar el burro con clic derecho aparecen **Acariciar (G)**, **Cepillar (B)** y **Alimentar (R)**, las mismas teclas que con un caballo. Cepillar lo limpia y alimentar gasta la comida y aplica sus efectos, igual que en el juego.
- **Cuidados montado:** acariciar (G) y alimentar funcionan como en un caballo. Para cepillar aparece **"Cepillar burro" (R)** cuando el burro está sucio, quieto y no estás en una misión.
- Responde al silbido, aparece en el mapa como tu caballo y tiene las opciones de dirigir, seguir, quedarse y huir.

### Mula y mula pintada

- **Vínculo natural**, que el juego original les bloqueaba: montar, alimentar, cepillar y acariciar dan XP y suben de nivel.
- **Ficha y estadísticas** en "Mostrar información" y `Personaje → Caballo`. Aparece en el mapa como tu caballo temporal.
- **Cuidados a pie:** una vez que tiene vínculo, la mula usa los botones originales del juego (acariciar, cepillar, alimentar).
- **Cuidados montado:** acariciar y alimentar como un caballo, y **"Cepillar mula" (R)** cuando está sucia.

## Limitaciones conocidas

- El motor del juego bloquea en el burro los botones originales de cuidados. Por eso el mod usa los suyos, con las mismas teclas. Después de cada acción a pie hay que soltar y volver a fijar al burro (clic derecho) para repetir.
- **No guardes mulas en el establo.** El juego original ya deja entrar a la mula, pero no tiene un item de catálogo para ella y la guarda rota: ficha sin raza, menú vacío y un hueco que no se puede seleccionar. Es un fallo del juego, no del mod. Si te pasa, carga una partida anterior.
- Los botones originales de la mula a pie a veces salen grises y otras no; aún no se sabe qué lo decide.
- El cepillo del menú TAB sale gris en burro y mula (en el caballo funciona); usa "Cepillar burro" / "Cepillar mula" montado.
- La opción "Quitar silla" no aparece en el burro ni en la mula.
- En "Mostrar información", el campo de pelaje del burro y la mula muestra el texto `~lp~` (no tienen texto de pelaje en el juego). Es solo estético.
- Compatibilidad con Stable Expansion: no probada.

## Requisitos

- Red Dead Redemption 2 **Story Mode** en PC, versión probada **1.0.1491.50** (Steam).
- [ScriptHookRDR2](http://www.dev-c.com/rdr2/scripthookrdr2/) (incluye el ASI loader).
- **No usar en Red Dead Online.**

El mod localiza en memoria lo que necesita mediante firmas y comparaciones, sin offsets fijos. En otra versión del juego, cualquier parte que no encuentre con seguridad se desactiva sola y lo indica en el log. Aun así, solo está probado en 1.0.1491.50.

## Instalación

1. Instala ScriptHookRDR2.
2. Copia `StabledDonkey.asi` (y, si quieres cambiar opciones, `StabledDonkey.ini`) en la carpeta del juego, junto a `RDR2.exe`.
3. Inicia Story Mode. Tras cargar la partida, el mod tarda unos 20 segundos en preparar los scripts. Si cargas otra partida, vuelve a hacerlo.

Si actualizas desde la v0.2.0 y tienes un `StabledDonkey.ini` propio, cambia `PatchMules=0` por `PatchMules=1` (o usa el `.ini` nuevo) para activar las mulas.

Para desinstalar, borra esos archivos. El mod no modifica archivos del juego ni tu partida más allá de lo que el propio juego guarda (por ejemplo, el burro en el establo). Todavía no se ha probado qué hace el juego con un burro guardado en el establo si se quita el mod: haz una copia de tu partida antes.

## Configuración

`StabledDonkey.ini` es opcional; sin él se usan los valores por defecto.

| Opción | Por defecto | Qué hace |
|---|---|---|
| `PatchDonkey` | `1` | Trata al burro como caballo (datos del modelo y scripts). |
| `PatchMules` | `1` | Lo mismo para la mula y la mula pintada. |
| `CarePrompts` | `1` | Botones de cuidado del mod (burro a pie; burro y mula montados). |
| `CareInputPat` / `CareInputBrush` / `CareInputFeed` | G / B / R | Controles de los botones a pie del burro. |
| `CareInputMountedBrush` | `INPUT_CONTEXT_X` (R) | Control de "Cepillar burro" / "Cepillar mula" montado. |
| `CareMountedBrushMinDirt` | `25` | Suciedad mínima (0–100) para mostrar el cepillado montado. |
| `CareMaxDistance` | `2.2` | Distancia máxima (m) para los botones a pie. |
| `HideVanillaPromptTypes` | `5,44,47,49,50,52` | Botones grises del juego que se ocultan en el burro. |

## Compilar desde el código fuente

Requisitos: Visual Studio 2022 o posterior (o Build Tools) con C++ y CMake, y el SDK de ScriptHookRDR2.

1. Descarga el SDK desde la página oficial y extráelo en `sdk/ScriptHookRDR2` (esa carpeta no se sube a Git; el SDK no se puede redistribuir).
2. Ejecuta `build.cmd`, o bien:

```powershell
cmake -S . -B build -A x64 -DSCRIPTHOOK_RDR2_SDK="C:/ruta/al/ScriptHookRDR2"
cmake --build build --config Release
```

El resultado es `build/Release/StabledDonkey.asi`.

## Cómo funciona

Resumen; el detalle está en [docs/NOTAS-TECNICAS.md](docs/NOTAS-TECNICAS.md).

- `src/native/ModelInfoPatch.cpp`: escribe en los datos del modelo del burro y de las mulas, en memoria, los valores del caballo (tipo de animal y otros campos). Los offsets se descubren comparando burro, mula y caballo.
- `src/native/ScriptPatch.cpp`: quita de los scripts cargados (`player_horse`, `shop_horse_shop_sp`) las comprobaciones explícitas "¿es un burro / una mula?". Respeta los datos de raza y catálogo que el juego usa para guardar y restaurar al burro, y deja intacta la comprobación de mula del establo.
- `src/native/CarePrompts.cpp`: los botones de cuidado. Cepillar y alimentar lanzan la misma secuencia de uso de objetos del juego.
- `src/main.cpp`: arranque, bucle principal y registro de la montura y del vínculo.

## Registro

El mod escribe `StabledDonkey.log` en la carpeta del juego. Se reinicia en cada arranque. Si informas de un problema, adjúntalo e indica qué hiciste y con qué animal.

## Créditos

- Scripts decompilados: [Halen84/RDR3-Decompiled-Scripts](https://github.com/Halen84/RDR3-Decompiled-Scripts).
- Base de datos de natives: [alloc8or/rdr3-nativedb-data](https://github.com/alloc8or/rdr3-nativedb-data).
- Controles y tipos de prompt: [femga/rdr3_discoveries](https://github.com/femga/rdr3_discoveries) y [Halen84/RDR3-Native-Flags-And-Enums](https://github.com/Halen84/RDR3-Native-Flags-And-Enums).
- Firma del lookup de modelos: proyecto RDR2Playground.

## Licencia

Este proyecto aún no incluye una licencia de reutilización. Todos los derechos del código fuente quedan reservados por su autor mientras no se publique una licencia explícita. Los componentes externos conservan sus propias condiciones.
