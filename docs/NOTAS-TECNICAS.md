# Notas técnicas

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

## 3. Botones de cuidado (`src/native/DonkeyCare.cpp`)

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
- Montado, "Cepillar burro" usa `INPUT_CONTEXT_X`, porque los controles `INPUT_INTERACT_*` no existen montado. Solo aparece con suciedad ≥ 25 (atributo 16), el burro quieto y sin misión.

Limitación: durante la animación el motor suelta el objetivo fijado y no lo recupera aunque se mantenga el clic derecho. No hay native para forzarlo.

## Probado en juego (v0.2.0)

- Vínculo natural con toast de XP; ficha "Mostrar información" y estadísticas en `Personaje → Caballo`.
- Establo: entra, guarda y devuelve el burro real con nombre, cola y ficha, también tras cerrar el juego.
- Montado: acariciar (G), alimentar y "Cepillar burro".
- A pie: acariciar, cepillar (limpia) y alimentar (gasta la comida); vínculo +1 cepillando o alimentando y +5 acariciando.
- Un burro salvaje (Rampage) se puede montar, gana vínculo, aparece en el mapa y responde al silbido.

## Mulas (pendiente)

- La mula es casi un caballo en `metapeds`. Solo cambian el tipo de animal (`AT_MULE`), `0x7BA672C7` (vacío), `0x3CF2AF47`, los floats, el flag y `0x2FC486F7`.
- `func_393` bloquea su vínculo y cuidados en `player_horse`.
- **No existe un item `HORSE_MULE`** en el catálogo, aunque `metapeds` lo referencia. Es el riesgo principal para el establo.
- `PatchMules=1` ya aplica a las mulas la parte del modelo y de los scripts. Falta comprobar si sus botones vanilla funcionan una vez que tienen vínculo, antes de añadir botones propios.

## Natives y globales usados

| Uso | Hash |
|---|---|
| `TASK_ANIMAL_INTERACTION` | `0xCD181A959CFDD7F4` |
| `_IS_ANIMAL_INTERACTION_RUNNING` | `0x7FC84E85D98F063D` |
| `GET_PLAYER_INTERACTION_TARGET_ENTITY` | `0x3EE1F7A8C32F24E1` |
| `_MODIFY_PLAYER_UI_PROMPT_FOR_PED` | `0xA3DB37EDF9A74635` |
| `_UI_PROMPT_*` (register, text, hold, group, visible, enabled) | ver `DonkeyCare.cpp` |
| `_INVENTORY_GET_INVENTORY_ITEM_COUNT_WITH_ITEMID` | `0xE787F05DFC977BDE` |
| `GET_ATTRIBUTE_BASE_RANK` (suciedad = 16) | `0x147149F2E909323C` |
| `GET_MISSION_FLAG` | `0xB15CD1CF58771DE1` |
| Global de uso de objetos de `player_horse` | `Global_1900372` |
