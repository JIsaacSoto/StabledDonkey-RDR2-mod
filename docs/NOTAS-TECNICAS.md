# Notas técnicas

[English](TECHNICAL-NOTES.md) | **Español**

Cómo funciona StabledDonkey por dentro, qué se probó y qué se descartó. Todo se refiere a RDR2 Story Mode PC **1.0.1491.50**. Los scripts del juego se consultaron decompilados (Halen84/RDR3-Decompiled-Scripts, carpeta 1491.50) y los natives en alloc8or/rdr3-nativedb-data.

## Idea general

El juego ya trae casi todo lo necesario para un burro "caballo":
- el catálogo (`catalog_sp.ymt`) tiene el item `HORSE_DONKEY` (`0xEF006BE0`, modelo `A_C_Donkey_01`);
- los scripts conocen `BREED_DONKEY`, la clase "Otro" y la relación `HORSE_DONKEY` ↔ `A_C_DONKEY_01`.

Lo que impide usarlo son unos pocos datos del modelo y unos chequeos explícitos en los scripts. El mod corrige ambos **en memoria**, sin tocar archivos del juego. Por eso el burro real se guarda en el establo y vuelve tal cual al cargar la partida: no hay ningún caballo sustituto.

## 1. Datos del modelo (`src/native/ModelInfoPatch.cpp`)

En `metapeds.ymt`, caballo, mula y burro difieren en campos del `CPedModelInfo` (0x380 bytes en esta versión). El módulo obtiene los objetos vivos del burro, la mula y el Tennessee Walker. Para eso usa la firma de lookup de modelos de RDR2Playground y verifica que el objeto contenga el hash del modelo. Luego **descubre los offsets comparándolos**: no hay offsets fijos en el código. Un campo solo se escribe si hay exactamente un candidato y el valor actual es el esperado.

| Campo (metapeds) | Caballo | Burro (original) | Offset hallado |
|---|---|---|---|
| `0x9ED31CFA` tipo de animal | `AT_HORSE` | `AT_DONKEY` | 0x31C |
| `0x7BA672C7` | `HORSE` | `DONKEY` | 0x310 |
| `0x3CF2AF47` | `HORSE` | `default` | 0x188 |
| `0xB6F6DE8E` (3 floats) | 25/50/100 | 0/0/0 | 0x360 |
| `0xF45D226F` (byte) | 1 | 0 | 0x36C |
| `MotionTaskDataSetName`, `TaskDataName` | `HORSE` | `DONKEY` | 0x2BC, 0x2D4 |
| `DecisionMakerName` | `0xC87F6A22` | `0x98D8BAF4` | 0x2D8 |
| `0x3207FDF6`, `0xC4E3A8E3`, `0x2799400B` | caballo | burro | 0x120, 0x2DC, 0x314 |

Las tareas y el DecisionMaker hicieron que se pudiera acariciar montado. El conjunto completo de la tabla es la configuración probada en juego. Con él el burro se mueve, suena y se anima con normalidad.

## 2. Chequeos de los scripts (`src/native/ScriptPatch.cpp`)

- `player_horse`: `func_392` (`switch` con `A_C_DONKEY_01`) y `func_393` (mulas) hacen que el script ignore vínculo, cuidados y estadísticas de esos animales.
- `shop_horse_shop_sp`: `GET_ENTITY_MODEL(montura) == A_C_DONKEY_01` ×3 → `TF_SHOP_HORSE_KICK_OUT_DONKEY` ("no puedes guardar un burro").

El módulo localiza cada script en memoria por un texto único (`DEAD_HORSE_SADDLE_SWAP_LONG_LOOT_ANIM_DATA`, `TF_SHOP_HORSE_KICK_OUT_DONKEY`). Después cambia solo dos formas de bytecode: los `switch` pequeños hechos solo de hashes de burro o mula, y las comparaciones agrupadas dentro del script del establo. El hash se sustituye por un marcador (`0x5D0D0001`…). Los demás usos del hash del burro (raza, catálogo) quedan intactos.

Opcodes reales de PC 1491.50 (distintos de las tablas públicas de consola): `SWITCH`=0x3C con conteo u16, `LOCAL_U8_LOAD`=0x66, `PUSH_CONST_U32`=0x37, `IEQ`=0x0B.

Detalles prácticos:
- El escaneo solo recorre memoria privada. La memoria mapeada o write-combined (GPU) es lentísima de leer y colgaba el escaneo. La pasada rápida tarda unos 20 s tras cargar.
- ScriptHook reinicia el `ScriptMain` del mod al cargar una partida, y el juego recarga sus scripts con los chequeos originales. Por eso `ScriptPatch::Reset()` vuelve a escanear en cada carga, y además se reescanea si un sitio parcheado cambia.

## 3. Botones de cuidado (`src/native/CarePrompts.cpp`)

A pie, el motor muestra **en gris** acariciar, cepillar y alimentar en el burro, y el cepillo del menú TAB también sale gris. Las interacciones en sí funcionan: `IS_ANIMAL_INTERACTION_POSSIBLE` = 1 y `TASK_ANIMAL_INTERACTION` se ejecuta.

Qué se descartó como causa del gris:
- Modos de prompt por ped o globales (`_GET_PLAYER_UI_PROMPT_FOR_PED_IS_ENABLED`): todos en 0.
- Propiedad de la montura (`_SET_PLAYER_OWNS_MOUNT`): se asigna, pero no cambia nada.
- Los datos del `CPedModelInfo` en los que el burro difiere de caballo y mula: valores simples, hashes de animación, audio, layout, IK y las referencias a fichas compartidas (0x1E0, 0x1F8, 0x200). Ninguno quita el gris. Solo quedaron sin probar la escala (0.67) y unos punteros propios del burro, que no tiene sentido cambiar.

Conclusión: el filtro está en el código del motor. Parchear el ejecutable se descartó por fragilidad y compatibilidad.

Solución: botones propios.
- A pie, dentro del menú de clic derecho del burro, con las teclas del caballo: `INPUT_INTERACT_LOCKON_ANIMAL` (G), `INPUT_INTERACT_HORSE_BRUSH` (B) y `INPUT_INTERACT_HORSE_FEED` (R). Los grises del juego se ocultan en ese burro con `_MODIFY_PLAYER_UI_PROMPT_FOR_PED(..., PP_MODE_HIDE)`.
- Cepillar y alimentar escriben la petición de la máquina de uso de objetos de `player_horse` (`Global_1900372`: [0] estado, 1 = libre; [1] item; [2] ped; [5] petición). Son las mismas escrituras que hace el manejador vanilla del prompt (`func_788`). Así el juego valida, lanza la animación, gasta el objeto y aplica limpieza, núcleos y vínculo.
- Acariciar usa `TASK_ANIMAL_INTERACTION` con `INTERACTION_POSITIVESHORT`.
- Comida: la misma lista que `player_horse` `func_789`. Primero 11 comidas de caballo (heno, galletas de avena, manzana…) y, si no queda ninguna, 26 hierbas (ginseng, salvia, menta…), todas aceptadas por su comprobación `func_724`. Hasta la v0.5.0 el mod solo tenía las 11 primeras y el botón desaparecía cuando solo quedaban hierbas.
- Textos de los botones: etiquetas del propio juego, leídas con `DOES_TEXT_LABEL_EXIST` (`0x73C258C68D6F55B6`) y `_GET_LABEL_TEXT_2` (`0x3429670F9B9EF2D3`); si faltan, texto en inglés. Comprobado en español (idioma 11 de `GET_CURRENT_LANGUAGE`): `INTERACT_HORSE` "Acariciar", `BRUSH_HORSE` "Cepillar", `FEED_HORSE` "Alimentar", `CALM_HORSE` "Tranquilizar", `REMOVE_SADDLE` "Quitar silla", `MOUNT_HORSE` "Montar caballo", `BREED_DONKEY` "Burro", `BREED_MULE` "Mula", `BREED_MULE_PAINTED` "Mula (manchada)". En inglés, `INTERACT_HORSE` es "Pat" (visto en los scripts decompilados); los demás textos en inglés no están comprobados. No hay etiqueta de "cepillar burro"; montado se usa `BRUSH_HORSE`.
- Montado, "Cepillar" usa `INPUT_CONTEXT_X`, porque los controles `INPUT_INTERACT_*` no existen montado. Solo aparece con suciedad ≥ 25 (atributo 16), el burro quieto y sin misión.

Limitación: durante la animación el motor suelta el objetivo fijado y no lo recupera aunque se mantenga el clic derecho. No hay native para forzarlo.

## Probado en juego (v0.2.0)

- Vínculo natural con toast de XP; ficha "Mostrar información" y estadísticas en `Personaje → Caballo`.
- Establo: entra, guarda y devuelve el burro real con nombre, cola y ficha, también tras cerrar el juego.
- Montado: acariciar (G), alimentar y "Cepillar".
- A pie: acariciar, cepillar (limpia) y alimentar (gasta la comida); vínculo +1 cepillando o alimentando y +5 acariciando.
- Un burro salvaje (Rampage) se puede montar, gana vínculo, aparece en el mapa y responde al silbido.

## Mulas (v0.3.0 a v0.5.0)

- La mula es casi un caballo en `metapeds`. Solo cambian el tipo de animal (`AT_MULE`), `0x7BA672C7` (vacío), `0x3CF2AF47`, los floats, el flag y `0x2FC486F7`. `ModelInfoPatch` les aplica los mismos campos base que al burro (no los exclusivos del burro).
- `player_horse` `func_393` bloqueaba su vínculo y cuidados; `ScriptPatch` lo neutraliza igual que `func_392`.
- Con vínculo, los botones originales a pie (acariciar, cepillar, alimentar) **funcionan en la mula**. Por eso el gris del burro es propio del burro (muy probablemente su esqueleto, `META_Base_Donkey_01`; la mula usa el de caballo) y no de los datos que compartimos.
- A veces esos botones de la mula salen grises (también con el gancho de la v0.1, y varía entre cargas). La causa no está identificada; en el diagnóstico, la mula aparecía como caballo temporal sin dueño registrado y sin modos de prompt activos. Desde la v0.4.0 la mula usa los botones del mod (`CareMuleOnFoot`).
- El cepillo del menú TAB sale gris en burro y mula y no en el caballo. Se probó darles la referencia de raza del caballo (`0xAAD9DD0F`, offset 0x370): no lo activa y hace que la ficha muestre las estadísticas del Tennessee Walker. Descartado; se usa "Cepillar" montado.
- **Establo (v0.4.0):**
  - Cada hueco del establo guarda el **modelo**. El item se calcula al vuelo con las funciones modelo → item (`shop_horse_shop_sp` `func_1318`, `player_horse` `func_542` y otras copias), y el menú oculta las filas cuyo item no es válido. No existe un item `HORSE_MULE`, así que en el juego original la mula se guarda sin fila ni título.
  - Esas funciones empiezan con `if (modelo == A_C_HORSE_MP_MANGY_BACKUP) return HORSE_BREED_MANGY_BACKUP;`, un caballo exclusivo del multijugador. En bytecode: `37 <0x30331B80> 15 08 00 37 <0x215F7CAE> 50 01 01` (el `if` usa el código combinado comparar-y-saltar `0x15`, no `IEQ`). El mod reescribe las dos constantes: mula → `HORSE_DONKEY`. Se aplicó en 6 sitios.
  - Resultado probado (mula pintada, 2026-11-26): fila y precio en el establo (título "Burro", raza "Mula"), montura principal, silbido, mapa y persistencia al cargar, fuera y dentro del establo.
  - En la v0.4.0 solo cabía un modelo (la mula normal).
  - El `switch` que sigue a esa comprobación está **ordenado por valor** (con signo), así que no se puede renombrar un caso sin romper el orden. Se probó reconstruir la tabla sacrificando un caballo real, y se descartó por su coste.
  - Tampoco se redirige el caso por defecto (`return 0`): los caballos de la banda (Bill, Charles, Dutch…), varios caballos especiales y el de multijugador tampoco tienen item, y el juego los deja fuera del establo a propósito. Con ese cambio se podrían guardar como "burro".
  - **v0.5.0 (probado 2026-11-26):** las 18 bytes del caso de multijugador se reescriben como un `switch` de dos casos: `66 00 | 3C 02 00 | <mula> <salto> | <mula pintada> <salto> | 13` (NOP). Los dos casos saltan al cuerpo ya existente del caso del burro (`37 <HORSE_DONKEY> 50 01 01`); el resto de modelos sigue al `switch` original sin cambios. La pila queda igual que antes.
  - Los saltos del `switch` cuentan desde el **final de cada entrada** (confirmado en el log: un solo criterio hace que los ~81 casos caigan en `37 xxxx 50 01 01`; el mod lo detecta en cada carga). `NOP` = `0x13` viene de la tabla de opcodes de maybegreat48/GTA-V-Script-Decompiler (`ShuffledOpcodes`, juego RDR2), que coincide con todos los opcodes ya vistos en memoria.
  - Resultado: mula pintada con fila en el establo, nombre y crin/cola cambiados en la tienda, montura principal, silbido, mapa y persistencia al cargar, fuera y dentro del establo; la mula normal y el burro siguen igual. En la test1 se escribió en 6 de 7 sitios. El séptimo (siempre en una dirección acabada en `…FE56`, a 0x3E56 de un bloque de 16 KB) no se reconoce y conserva el cambio de la v0.4.0 (solo mula normal). El log de la test2 dio el motivo: con la base correcta, los casos 0–78 de 81 caen en `return const`, pero el caso 79 (`A_C_HORSE_APPALOOSA_BLANKET`) salta 3 bytes fuera del patrón de los demás cuerpos. Desde la test3 se toleran hasta 2 casos así si el caso del burro es exacto (es el único que se usa), y el log muestra los bytes del caso raro. Resultado de la test3 (probado 2026-11-26): ese caso empieza con `68 02 00` (J +2), salta dos `13` (NOP) y llega a un `37 xxxx 50 01 01` normal, es decir, relleno del compilador (probablemente un límite de página de código). Con eso se escriben los 7 sitios (`both_mules` 7–8, `mule_only=0`).
  - La comprobación de mula del propio script del establo (`func_2186`) se deja intacta. Está en la misma región de memoria que el texto marcador del establo, a diferencia de las de `player_horse`.
- **Desinstalar (probado 2026-11-26):** partida con la mula como montura principal y un burro, la mula pintada y un caballo en el establo. Sin el mod, el menú "Gestionar caballos" sale vacío (hipótesis: el script del establo no sabe pintar esos huecos y aborta la lista completa). La mula principal aparece, sus interacciones originales funcionan sin dar vínculo, su ficha es de animal común y el vínculo baja a nivel 1. Al reinstalar el mod todo vuelve a funcionar (salvo la mula pintada, como antes de la v0.5.0).
- Cuando el jugador se queda sin montura principal, `player_horse` usa un caballo por defecto (`func_970`: Tennessee Walker castaño). No es cosa del mod.
- "Quitar silla" (H, `INPUT_SADDLE_TRANSFER`) es un botón del motor sin native ni tarea equivalente. `player_horse` solo recibe `EVENT_SADDLE_TRANSFER` cuando ya ocurrió. No aparece ni con la mula como montura principal, así que no depende del esqueleto.

## SuperDonkMode (v0.6.0, `src/native/SuperDonk.cpp`)

- Las estadísticas de un caballo son rangos de atributos (`ePedAttribute`: 0 salud, 1 energía, 5 velocidad, 6 aceleración). Los scripts nunca fijan el rango base de esas cuatro: el motor lo toma de la raza. Las sillas suman rangos de **bonificación** a velocidad y aceleración, y `player_horse` reescribe esos rangos de bonificación de la montura del jugador según su equipo en cada actualización (`func_662`–`func_664`). Por eso el mod fija el rango **base** (`SET_ATTRIBUTE_BASE_RANK`, `0x5DA12E025D47D4E5`) y el equipo se sigue sumando encima.
- El establo muestra cada estadística como rango base + 1 sobre 10 (`shop_horse_shop_sp`, `"HorseSpeedValue"`). Rangos de fábrica del propio juego, leídos en el juego con `GET_DEFAULT_ATTRIBUTE_RANK` (`0x958DD43D41F89A47`): Árabe blanco salud 4, energía 4, velocidad 5, aceleración 5; Turkmeno bayo oscuro 6, 4, 5, 4 (rango máximo 9). Sumando 1 coinciden con las guías publicadas (5/5/6/6 y 7/5/6/5). Burro y mulas: 0 en las cuatro.
- El mod revisa una vez por segundo el burro o la mula que se monta y el último montado, y vuelve a fijar los rangos si no coinciden.
- Probado en el juego (2026-11-26):
  - Caballos y mulas: salud máxima y energía máxima = 60 + 20 × rango (caballo normal de rango 2: 100 / 100; mula con aumento de rango 6 / 4: 180 / 140).
  - El menú TAB y "Mostrar información" muestran las estadísticas mejoradas; el establo y `Personaje → Caballo` muestran las de fábrica de la raza.
  - El burro acepta los cuatro rangos (velocidad y aceleración se aplican), pero su salud máxima (251) y su energía máxima (300) no cambian. Al galopar todos gastan unos 6,3 puntos de energía cada 5 s. Lo que cansa al animal es la energía normalizada (`_GET_PED_STAMINA_NORMALIZED`), cuyo rango oculto es de unos 60 en un caballo de rango 2, unos 100 en una mula de rango 4 y unos 104–107 en el burro **con o sin** el aumento. Es decir, la energía del burro está fija en más o menos un rango 4, y su salud supera la de cualquier rango de caballo (rango 9 = 240).
  - No hay native para fijar la energía máxima. `_SET_STAMINA_DEPLETION_MULTIPLIER` podría emular más energía en el burro, pero `player_horse` la reescribe en la montura del jugador según el equipo; se dejó fuera.

## Natives y globales usados

| Uso | Hash |
|---|---|
| `TASK_ANIMAL_INTERACTION` | `0xCD181A959CFDD7F4` |
| `_IS_ANIMAL_INTERACTION_RUNNING` | `0x7FC84E85D98F063D` |
| `GET_PLAYER_INTERACTION_TARGET_ENTITY` | `0x3EE1F7A8C32F24E1` |
| `_MODIFY_PLAYER_UI_PROMPT_FOR_PED` | `0xA3DB37EDF9A74635` |
| `_UI_PROMPT_*` (register, text, hold, group, visible, enabled) | ver `CarePrompts.cpp` |
| `_INVENTORY_GET_INVENTORY_ITEM_COUNT_WITH_ITEMID` | `0xE787F05DFC977BDE` |
| `GET_ATTRIBUTE_BASE_RANK` (suciedad = 16) | `0x147149F2E909323C` |
| `GET_MISSION_FLAG` | `0xB15CD1CF58771DE1` |
| `DOES_TEXT_LABEL_EXIST` | `0x73C258C68D6F55B6` |
| `_GET_LABEL_TEXT_2` | `0x3429670F9B9EF2D3` |
| `GET_CURRENT_LANGUAGE` | `0xDB917DA5C6835FCC` |
| `SET_ATTRIBUTE_BASE_RANK` | `0x5DA12E025D47D4E5` |
| `GET_MAX_ATTRIBUTE_RANK` | `0x704674A0535A471D` |
| `GET_DEFAULT_ATTRIBUTE_RANK` | `0x958DD43D41F89A47` |
| `GET_PED_MAX_HEALTH` | `0x4700A416E8324EF3` |
| `_GET_PED_MAX_STAMINA` | `0xCB42AFE2B613EE55` |
| Global de uso de objetos de `player_horse` | `Global_1900372` |
