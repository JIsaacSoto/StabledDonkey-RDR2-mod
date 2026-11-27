# StabledDonkey

[English](README.md) | **Español**

**Burros y mulas como caballos en Red Dead Redemption 2 (Story Mode): vínculo, cuidados, estadísticas, establo y persistencia.**

StabledDonkey hace que el burro y la mula funcionen como una raza de caballo más, usando los propios sistemas del juego. No reemplaza sus modelos por caballos ni modifica archivos del juego: todos los cambios se aplican en memoria al arrancar.

## Qué hace (v0.7.0)

### Burro

- **Vínculo natural:** montar, alimentar, cepillar y acariciar dan XP de vínculo, con el toast y las subidas de nivel del juego.
- **Ficha y estadísticas:** "Mostrar información" y `Personaje → Caballo` muestran la ficha del burro con sus datos.
- **Establo:** puedes guardar el burro, elegirlo como montura principal y recuperarlo. Vuelve el burro real, con su nombre y su aspecto, también después de cerrar el juego.
- **Cuidados a pie:** al fijar el burro con clic derecho aparecen **Acariciar (G)**, **Cepillar (B)** y **Alimentar (R)**, las mismas teclas que con un caballo. Como en el juego original, acariciar está disponible desde el principio (es la forma de empezar el vínculo) y cepillar y alimentar piden vínculo. Cepillar lo limpia y alimentar gasta la comida y aplica sus efectos, igual que en el juego. Basta con una pulsación y puedes repetir sin dejar de fijarlo.
- **Cuidados montado:** acariciar (G) y alimentar funcionan como en un caballo. Para cepillar aparece **"Cepillar" (R)** (mantener pulsado) cuando el burro está sucio, quieto y no estás en una misión.
- Alimentar usa la misma comida que el juego: primero la de caballo y, si no te queda, las hierbas que el juego también acepta (ginseng, salvia, menta…).
- Los textos de los botones son los del propio juego, así que salen en el idioma que tengas.
- Responde al silbido, aparece en el mapa como tu caballo y tiene las opciones de dirigir, seguir, quedarse y huir.

### Mula y mula pintada

- **Vínculo natural**, que el juego original les bloqueaba: montar, alimentar, cepillar y acariciar dan XP y suben de nivel.
- **Ficha y estadísticas** en "Mostrar información" y `Personaje → Caballo`. Aparece en el mapa como tu caballo.
- **Establo:** la mula y la mula pintada se guardan, se pueden elegir como montura principal, responden al silbido y se conservan al cargar la partida, tanto fuera como dentro del establo. En el establo su título dice "Burro" (usan el item del burro), pero su raza dice "Mula" o "Mula (manchada)".
- **Cuidados a pie:** **Acariciar (G)**, **Cepillar (B)** y **Alimentar (R)** del mod, porque los originales de la mula a veces salen grises. Acariciar está disponible desde el principio; cepillar y alimentar piden vínculo.
- **Cuidados montado:** acariciar y alimentar como un caballo, y **"Cepillar" (R)** cuando está sucia.

### SuperDonkMode (opcional)

Desactivado por defecto (`SuperDonkMode=0`). Al activarlo, el burro o la mula que montas mejora sus estadísticas con las funciones de atributos del propio juego, como un mod de mejora. Por defecto recibe la velocidad y la aceleración del Árabe blanco y la salud y la energía del Turkmeno bayo oscuro; todos los valores se pueden cambiar en el `.ini`.

- **Mulas:** se aplican las cuatro (salud, energía, velocidad y aceleración).
- **Burro:** se aplican velocidad y aceleración. Su salud y su energía vienen fijas en su modelo y no cambian, pero ya son más o menos las de un Turkmeno (energía) o más (salud).
- El menú TAB y "Mostrar información" muestran las estadísticas mejoradas. El establo y `Personaje → Caballo` muestran las originales de la raza.
- Las bonificaciones de la silla se suman encima, como en cualquier caballo.

## Limitaciones conocidas

- En el establo, el título de las dos mulas dice "Burro": el juego no tiene un item de catálogo para las mulas y el mod les da el del burro. La raza sí dice "Mula".
- El cepillo del menú TAB sale gris en burro y mula (en el caballo funciona); usa "Cepillar" montado.
- La opción "Quitar silla" no aparece en el burro ni en la mula, ni siquiera como montura principal.
- En "Mostrar información", el campo de pelaje del burro y la mula muestra el texto `~lp~` (no tienen texto de pelaje en el juego). Es solo estético.
- SuperDonkMode no puede subir la salud ni la energía del burro (ver arriba).
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

Si actualizas y tienes un `StabledDonkey.ini` propio, usa el `.ini` nuevo. La línea `StableMuleModel` de la v0.4.0 se ignora y se puede borrar.

### Desinstalar

**Antes de quitar el mod, saca del establo a todos los burros y mulas** (o véndelos). Después borra `StabledDonkey.asi` y `StabledDonkey.ini`.

El mod no modifica archivos del juego. Lo único que queda en tu partida es lo que el propio juego guarda: los animales del establo y la montura principal. Probado sin el mod (2026-11-26):

- **Con un burro o una mula dentro del establo, el menú "Gestionar caballos" queda vacío** (solo deja retroceder). Sin el mod el juego no sabe mostrarlos. Al volver a instalar el mod, el establo vuelve a funcionar.
- Una mula como montura principal sigue apareciendo y acepta las interacciones originales, pero ya no da puntos de vínculo, su ficha es la de un animal común y su vínculo vuelve al nivel 1.

## Configuración

`StabledDonkey.ini` es opcional; sin él se usan los valores por defecto. Sus comentarios están en inglés y en español.

| Opción | Por defecto | Qué hace |
|---|---|---|
| `PatchDonkey` | `1` | Trata al burro como caballo (datos del modelo y scripts). |
| `PatchMules` | `1` | Lo mismo para la mula y la mula pintada. |
| `StableMules` | `1` | Establo y persistencia para la mula y la mula pintada. |
| `CarePrompts` | `1` | Botones de cuidado del mod (a pie; cepillado montado). |
| `CareMuleOnFoot` | `1` | Botones a pie del mod también para la mula (0 = los originales). |
| `CareInputPat` / `CareInputBrush` / `CareInputFeed` | G / B / R | Controles de los botones a pie. |
| `CareInputMountedBrush` | `INPUT_CONTEXT_X` (R) | Control de "Cepillar" montado. |
| `CareMountedBrushMinDirt` | `25` | Suciedad mínima (0–100) para mostrar el cepillado montado. |
| `CareMaxDistance` | `2.2` | Distancia máxima (m) para los botones a pie. |
| `HideVanillaPromptTypes` | `5,44,47,49,50,52` | Botones del juego que se ocultan en el burro. |
| `HideVanillaPromptTypesMule` | `49,50` | Botones del juego que se ocultan en la mula (solo cepillar y alimentar). |
| `CarePromptPriority` | `2` | Prioridad de los botones del mod frente a los del juego (0–3). |
| `SuperDonkMode` | `0` | Mejora de estadísticas del burro y las mulas que montas (1 = activado). |
| `SuperDonkHealth` / `SuperDonkStamina` | `6` / `4` | Rango de salud y energía (0–9; el establo muestra rango + 1). Por defecto: Turkmeno bayo oscuro. Solo mulas. `-1` = sin cambios. |
| `SuperDonkSpeed` / `SuperDonkAcceleration` | `5` / `5` | Rango de velocidad y aceleración (0–9). Por defecto: Árabe blanco. `-1` = sin cambios. |

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

Resumen; el detalle está en [docs/NOTAS-TECNICAS.md](docs/NOTAS-TECNICAS.md) ([English](docs/TECHNICAL-NOTES.md)).

- `src/native/ModelInfoPatch.cpp`: escribe en los datos del modelo del burro y de las mulas, en memoria, los valores del caballo (tipo de animal y otros campos). Los offsets se descubren comparando burro, mula y caballo.
- `src/native/ScriptPatch.cpp`: quita de los scripts cargados (`player_horse`, `shop_horse_shop_sp`) las comprobaciones explícitas "¿es un burro / una mula?" y da a las dos mulas un item de catálogo para el establo. Respeta los datos de raza y catálogo que el juego usa para guardar y restaurar a los animales.
- `src/native/CarePrompts.cpp`: los botones de cuidado. Cepillar y alimentar lanzan la misma secuencia de uso de objetos del juego.
- `src/native/SuperDonk.cpp`: SuperDonkMode, la mejora opcional de estadísticas.
- `src/main.cpp`: arranque, bucle principal y registro de la montura y del vínculo.

## Registro

El mod escribe `StabledDonkey.log` en la carpeta del juego. Se reinicia en cada arranque. Si informas de un problema, adjúntalo e indica qué hiciste y con qué animal.

## Créditos

- Scripts decompilados: [Halen84/RDR3-Decompiled-Scripts](https://github.com/Halen84/RDR3-Decompiled-Scripts).
- Base de datos de natives: [alloc8or/rdr3-nativedb-data](https://github.com/alloc8or/rdr3-nativedb-data).
- Controles y tipos de prompt: [femga/rdr3_discoveries](https://github.com/femga/rdr3_discoveries) y [Halen84/RDR3-Native-Flags-And-Enums](https://github.com/Halen84/RDR3-Native-Flags-And-Enums).
- Tabla de opcodes de los scripts: [maybegreat48/GTA-V-Script-Decompiler](https://github.com/maybegreat48/GTA-V-Script-Decompiler).
- Nombres de las etiquetas de texto: [OpenIV-Team/RAGE-StringsDatabase](https://github.com/OpenIV-Team/RAGE-StringsDatabase).
- Firma del lookup de modelos: proyecto RDR2Playground.

## Licencia

Este proyecto aún no incluye una licencia de reutilización. Todos los derechos del código fuente quedan reservados por su autor mientras no se publique una licencia explícita. Los componentes externos conservan sus propias condiciones.
