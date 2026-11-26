# StabledDonkey

**Burros y mulas como caballos en Red Dead Redemption 2 (Story Mode): vínculo, cuidados, estadísticas, establo y persistencia.**

StabledDonkey hace que el burro y la mula funcionen como una raza de caballo más, usando los propios sistemas del juego. No reemplaza sus modelos por caballos ni modifica archivos del juego: todos los cambios se aplican en memoria al arrancar.

## Qué hace (v0.4.0)

### Burro

- **Vínculo natural:** montar, alimentar, cepillar y acariciar dan XP de vínculo, con el toast y las subidas de nivel del juego.
- **Ficha y estadísticas:** "Mostrar información" y `Personaje → Caballo` muestran la ficha del burro con sus datos.
- **Establo:** puedes guardar el burro, elegirlo como montura principal y recuperarlo. Vuelve el burro real, con su nombre y su aspecto, también después de cerrar el juego.
- **Cuidados a pie:** al fijar el burro con clic derecho aparecen **Acariciar (G)**, **Cepillar (B)** y **Alimentar (R)**, las mismas teclas que con un caballo. Como en el juego original, acariciar está disponible desde el principio (es la forma de empezar el vínculo) y cepillar y alimentar piden vínculo. Cepillar lo limpia y alimentar gasta la comida y aplica sus efectos, igual que en el juego.
- **Cuidados montado:** acariciar (G) y alimentar funcionan como en un caballo. Para cepillar aparece **"Cepillar burro" (R)** cuando el burro está sucio, quieto y no estás en una misión.
- Responde al silbido, aparece en el mapa como tu caballo y tiene las opciones de dirigir, seguir, quedarse y huir.

### Mula y mula pintada

- **Vínculo natural**, que el juego original les bloqueaba: montar, alimentar, cepillar y acariciar dan XP y suben de nivel.
- **Ficha y estadísticas** en "Mostrar información" y `Personaje → Caballo`. Aparece en el mapa como tu caballo.
- **Establo (mula normal):** se guarda, se puede elegir como montura principal, responde al silbido y se conserva al cargar la partida, tanto fuera como dentro del establo. En el establo su título dice "Burro" (usa el item del burro), pero su raza dice "Mula".
- **Cuidados a pie:** **Acariciar (G)**, **Cepillar (B)** y **Alimentar (R)** del mod, porque los originales de la mula a veces salen grises. Acariciar está disponible desde el principio; cepillar y alimentar piden vínculo.
- **Cuidados montado:** acariciar y alimentar como un caballo, y **"Cepillar mula" (R)** cuando está sucia.

## Limitaciones conocidas

- Los botones de cuidado a pie son del mod. Después de cada acción a pie hay que soltar y volver a fijar al animal (clic derecho) para repetir.
- **No guardes la mula pintada en el establo** (solo aparece en una misión). El juego no tiene un item de catálogo para las mulas; el mod da a la mula normal el del burro, pero solo cabe un modelo. La pintada sigue con el fallo del juego original: ficha sin título, menú vacío y un hueco que no se puede seleccionar. Si te pasa, carga una partida anterior. Puedes elegir cuál de las dos se guarda con `StableMuleModel`.
- El cepillo del menú TAB sale gris en burro y mula (en el caballo funciona); usa "Cepillar burro" / "Cepillar mula" montado.
- La opción "Quitar silla" no aparece en el burro ni en la mula, ni siquiera como montura principal.
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

Si actualizas desde una versión anterior y tienes un `StabledDonkey.ini` propio, usa el `.ini` nuevo (o añade `PatchMules=1`, `StableMules=1` y `CareMuleOnFoot=1`).

Para desinstalar, borra esos archivos. El mod no modifica archivos del juego ni tu partida más allá de lo que el propio juego guarda (por ejemplo, el burro en el establo). Todavía no se ha probado qué hace el juego con un burro guardado en el establo si se quita el mod: haz una copia de tu partida antes.

## Configuración

`StabledDonkey.ini` es opcional; sin él se usan los valores por defecto.

| Opción | Por defecto | Qué hace |
|---|---|---|
| `PatchDonkey` | `1` | Trata al burro como caballo (datos del modelo y scripts). |
| `PatchMules` | `1` | Lo mismo para la mula y la mula pintada. |
| `StableMules` | `1` | Establo y persistencia para un modelo de mula. |
| `StableMuleModel` | `mule` | Qué mula se puede guardar: `mule` (normal) o `painted` (pintada). |
| `CarePrompts` | `1` | Botones de cuidado del mod (a pie; cepillado montado). |
| `CareMuleOnFoot` | `1` | Botones a pie del mod también para la mula (0 = los originales). |
| `CareInputPat` / `CareInputBrush` / `CareInputFeed` | G / B / R | Controles de los botones a pie. |
| `CareInputMountedBrush` | `INPUT_CONTEXT_X` (R) | Control de "Cepillar burro" / "Cepillar mula" montado. |
| `CareMountedBrushMinDirt` | `25` | Suciedad mínima (0–100) para mostrar el cepillado montado. |
| `CareMaxDistance` | `2.2` | Distancia máxima (m) para los botones a pie. |
| `HideVanillaPromptTypes` | `5,44,47,49,50,52` | Botones del juego que se ocultan en el burro. |
| `HideVanillaPromptTypesMule` | `49,50` | Botones del juego que se ocultan en la mula (solo cepillar y alimentar). |
| `CarePromptPriority` | `2` | Prioridad de los botones del mod frente a los del juego (0–3). |

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
- `src/native/ScriptPatch.cpp`: quita de los scripts cargados (`player_horse`, `shop_horse_shop_sp`) las comprobaciones explícitas "¿es un burro / una mula?" y da a la mula un item de catálogo para el establo. Respeta los datos de raza y catálogo que el juego usa para guardar y restaurar a los animales.
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
