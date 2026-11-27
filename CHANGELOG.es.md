# Registro de cambios

[English](CHANGELOG.md) | **Español**

## v0.7.0 — 2026-11-27

Cuidados a pie sin volver a fijar al animal.

### Cambiado

- Los botones a pie Acariciar, Cepillar y Alimentar se activan con una pulsación simple, como los del juego. "Cepillar" montado sigue siendo de mantener pulsado, para no activarlo sin querer al montar.

### Corregido

- Después de cada acción a pie los botones quedaban en gris hasta volver a fijar al animal (clic derecho). El juego no suelta el fijado, como se creía: mantiene la interacción con el animal "en curso" mientras se mantiene fijado, y el mod esperaba a que terminara. Ahora los botones vuelven en cuanto termina la acción (2,5 s, y en Cepillar y Alimentar cuando el juego termina de usar el objeto). Probado con el burro, la mula y la mula pintada.
- El estado de uso de objetos de `player_horse` se lee como valor de 32 bits (durante el cepillado se vieron restos en los bits altos).

## v0.6.0 — 2026-11-26

SuperDonkMode: una mejora opcional de estadísticas para el burro y las mulas.

### Añadido

- `SuperDonkMode` (desactivado por defecto): sube los rangos base de atributos del burro o la mula que monta el jugador con las funciones de atributos del juego (`SET_ATTRIBUTE_BASE_RANK`). Los valores por defecto son los rangos de fábrica del propio juego: velocidad 5 y aceleración 5 del Árabe blanco, salud 6 y energía 4 del Turkmeno bayo oscuro (el establo muestra rango + 1: 6 / 6 y 7 / 5). Todos los valores se ajustan en el ini (`SuperDonkHealth`, `SuperDonkStamina`, `SuperDonkSpeed`, `SuperDonkAcceleration`; `-1` deja una estadística sin cambios). Las bonificaciones de la silla se siguen sumando encima.
- Probado: en las mulas se aplican las cuatro estadísticas (salud máxima 60 → 180, energía máxima 60 → 140). El menú TAB y "Mostrar información" muestran las estadísticas mejoradas; el establo y `Personaje → Caballo` muestran las originales de la raza.

### Limitaciones conocidas

- En el burro solo se aplican velocidad y aceleración. Su salud máxima (251) y su energía vienen fijas en su modelo y no siguen los rangos; probado en el juego, su energía ya dura más o menos como un rango 4 (la del Turkmeno) y su salud es mayor que la de cualquier rango de caballo (rango 9 = 240).

## v0.5.0 — 2026-11-26

La mula pintada entra en el establo, los botones usan los textos del propio juego y la documentación está ahora en inglés y en español.

### Añadido

- La mula pintada también se puede guardar en el establo, junto con la mula normal (las dos usan el item `HORSE_DONKEY`). El caso exclusivo del multijugador al principio de cada función modelo → item se reescribe como un `switch` de dos casos (mula, mula pintada) que salta al caso ya existente del burro; el resto de caballos sigue al `switch` original, así que los caballos sin item (los de la banda y otros) se siguen rechazando como en el juego original. Probado: fila en el establo, cambio de nombre y de crin / cola, montura principal, silbido, mapa y persistencia al cargar, dentro y fuera del establo.
- Los textos de los botones de cuidado salen de las etiquetas de texto del propio juego (`INTERACT_HORSE`, `BRUSH_HORSE`, `FEED_HORSE`), así que siguen el idioma del juego. Si falta una etiqueta, se usa el texto en inglés.

### Cambiado

- El botón de cepillar montado muestra el texto de cepillar del juego (`BRUSH_HORSE`, "Cepillar" en español) en lugar de los textos fijos "Cepillar burro" / "Cepillar mula".
- Documentación en inglés y en español: `README.md` / `README.es.md`, `CHANGELOG.md` / `CHANGELOG.es.md`, `docs/TECHNICAL-NOTES.md` / `docs/NOTAS-TECNICAS.md`, comentarios bilingües en `StabledDonkey.ini` y `README.txt` / `LEEME.txt` en el paquete de la versión.

### Eliminado

- `StableMuleModel`: ahora se guardan las dos mulas.

### Corregido

- Alimentar no aparecía cuando solo quedaban hierbas. Ahora usa la misma lista de comida que el juego: primero la comida de caballo y después las 26 hierbas a las que recurre el juego (ginseng, salvia, menta…).
- Una de las siete funciones modelo → item no se reconocía (uno de sus casos salta sobre relleno del compilador), así que solo conocía la mula normal. Ahora se toleran hasta 2 casos raros si el caso del burro es exacto; se reescriben las siete.

### Documentado

- Prueba de desinstalación: sin el mod, un establo con un burro o una mula muestra vacío el menú "Gestionar caballos" (vuelve a funcionar al reinstalar el mod). Una mula como montura principal sigue apareciendo con sus interacciones originales, pero no da puntos de vínculo, su ficha es la de un animal común y su vínculo baja al nivel 1. Saca a los burros y mulas del establo (o véndelos) antes de desinstalar.

## v0.4.0 — 2026-11-26

La mula normal ya se puede guardar en el establo, elegir como montura principal y conservar al cargar la partida.

### Añadido

- Establo para la mula (`StableMules=1`). El juego no tiene un item de catálogo para las mulas, así que una mula guardada se quedaba sin fila en el menú (un fallo del juego original). Las funciones modelo → item (`player_horse`, `shop_horse_shop_sp` y otros scripts con el mismo código) empiezan con un caso para un caballo exclusivo del multijugador; ese caso se reescribe en memoria para dar a la mula el item `HORSE_DONKEY`. La mula tiene así su fila en el establo, se puede elegir como montura principal, responde al silbido, aparece en el mapa y se restaura al cargar. Su raza sigue diciendo "Mula".
- `StableMuleModel` elige qué mula ocupa ese único hueco: `mule` (por defecto) o `painted`.
- Botones de cuidado a pie del mod (G acariciar, B cepillar, R alimentar) para las mulas (`CareMuleOnFoot=1`), porque sus botones originales salen grises de forma impredecible. En las mulas solo se ocultan el cepillar y el alimentar originales (`HideVanillaPromptTypesMule=49,50`): ocultarlos todos hacía que el motor soltara al animal fijado.
- `CarePromptPriority` (por defecto 2, alta) para que los botones originales activos no tapen los del mod.

### Cambiado

- Como en los caballos del juego, acariciar a pie se ofrece antes del vínculo (es lo que lo empieza), mientras que cepillar y alimentar piden vínculo de nivel 1.

### Limitaciones conocidas

- Solo cabe un modelo de mula en el establo. La mula pintada (solo aparece en una misión) conserva el fallo del juego original.
- En el establo, el título de la mula dice "Burro" (usa el item del burro).
- Después de una acción a pie hay que volver a fijar al animal (clic derecho) para repetir; ahora también en las mulas.
- "Quitar silla" sigue sin aparecer, ni siquiera con la mula como montura principal. El cepillo del menú TAB sigue gris.

## v0.3.0 — 2026-11-25

Las mulas se unen al burro: vínculo natural y cuidados para la mula y la mula pintada.

### Añadido

- Soporte para la mula y la mula pintada, activado por defecto (`PatchMules=1`). El mismo cambio de datos del modelo en memoria, y además se quita la comprobación de mula de `player_horse`, así que las mulas ganan XP de vínculo al montarlas, alimentarlas, cepillarlas y acariciarlas, suben de nivel, muestran su ficha y estadísticas y aparecen como caballo temporal.
- Con vínculo, las mulas usan los botones originales a pie de acariciar / cepillar / alimentar.
- "Cepillar mula" (R) al montar una mula sucia, como "Cepillar burro".

### Cambiado

- El módulo de los botones de cuidado es ahora `src/native/CarePrompts.cpp` (antes `DonkeyCare.cpp`).
- La comprobación de mula del propio script del establo se deja intacta, así que el establo trata a las mulas exactamente como el juego original.

### Limitaciones conocidas

- No guardes mulas en el establo. El juego original ya deja entrar a una mula, pero no hay item de catálogo para ella y la guarda rota (sin raza, menú en blanco, hueco que no se puede seleccionar). Es un fallo del juego original.
- Los botones originales a pie de la mula salen grises a veces; aún no se conoce la causa.
- El cepillo del menú TAB sigue gris en el burro y las mulas (en los caballos funciona).
- El campo de pelaje de la ficha muestra `~lp~` en el burro y las mulas (estético).

## v0.2.0 — 2026-11-25

El burro funciona ahora como una raza de caballo de verdad, con todos los cambios aplicados en memoria y sin tocar archivos del juego.

### Añadido

- Cambio de los datos del modelo en memoria: los datos del modelo del burro reciben el tipo de animal del caballo y los campos relacionados. Los offsets se descubren al vuelo comparando los datos del modelo del burro, la mula y el caballo.
- Cambio de los scripts en memoria: quita las comprobaciones "¿es un burro?" del bytecode cargado de `player_horse` y `shop_horse_shop_sp`. Los datos de raza y catálogo quedan intactos. Los scripts se vuelven a cambiar después de cada carga de partida y siempre que el juego los recarga.
- Establo: el burro real se puede guardar, elegir como montura principal y recuperar con su nombre y su aspecto, también después de reiniciar el juego.
- Botones de cuidado para el burro. A pie, Acariciar (G), Cepillar (B) y Alimentar (R) aparecen en el menú de fijar. Montado, aparece "Cepillar burro" (R) cuando el burro está sucio. Cepillar y alimentar lanzan la secuencia de uso de objetos de `player_horse`: limpieza, uso del objeto, núcleos y vínculo.
- `StabledDonkey.ini` opcional con las opciones documentadas.
- `docs/NOTAS-TECNICAS.md` con los hallazgos técnicos.

### Eliminado

- El sustituto Tennessee Walker de la v0.1 (gancho de `GET_ENTITY_MODEL`) y el puente experimental del establo (`StabledDonkey.bridge`, intercambio de entidad con F7). Ya no hacen falta y hacían que aparecieran el nombre y las estadísticas del Tennessee Walker.

### Limitaciones conocidas

- El motor mantiene en gris los botones de cuidado originales y el cepillo del menú TAB en el burro. Los botones del mod los sustituyen, pero después de cada acción a pie hay que volver a fijar al burro.
- "Quitar silla" no está disponible en el burro.
- Las mulas aún no tienen soporte (`PatchMules=0`).
- Solo comprobado con RDR2 Steam 1.0.1491.50.

## v0.1.0 — 2026-10-09

Primera versión pública, centrada en las recompensas naturales de vínculo para los modelos de burro y mula compatibles.

- La lógica de recompensas de cuidado del caballo del juego reconoce los modelos compatibles durante la interacción activa con la montura.
- Se conservan las actualizaciones de puntos de vínculo, el toast de XP y el aviso de subida de nivel del propio juego; el mod no añade puntos de vínculo ni dibuja un toast propio.
- Registro del modelo de la montura, los puntos de vínculo y el nivel de vínculo en `StabledDonkey.log`.
- Vínculo del burro confirmado alimentándolo montado, cepillándolo con el cepillo y montándolo en Story Mode.
- Confirmado que el burro puede aparecer como caballo temporal y usar la ficha de información del caballo tras el vínculo.

### Limitaciones conocidas

- Las acciones a pie del burro (alimentar, cepillar, acariciar) siguen desactivadas o no aparecen.
- La persistencia en el establo y la compatibilidad con Stable Expansion no están implementadas.
- Algunas partes de la interfaz pueden identificar al burro como un Tennessee Walker.
- El gancho de native actual, específico de la versión del juego, solo está comprobado con RDR2 Steam 1.0.1491.50.
