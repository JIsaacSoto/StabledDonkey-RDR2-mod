# StabledDonkey

**V0.1 — bonding natural para burros y mulas en RDR2 Story Mode.**

StabledDonkey adapta una parte del sistema vanilla de caballos para que los burros y las mulas puedan ganar vínculo de forma natural. No reemplaza sus modelos por caballos.

## Estado de V0.1

### Verificado en juego

- El burro gana XP de vínculo al alimentarlo montado, cepillarlo con el cepillo y montar durante un tiempo.
- Aparecen el toast de XP y la notificación vanilla al alcanzar el nivel 1.
- El burro pasa a figurar como montura temporal en el mapa tras ganar vínculo.
- `Personaje → Caballo` puede mostrar la ficha vanilla del burro con sus estadísticas y vínculo.
- Silbido, montar, dirigir y varias funciones vanilla de caballo funcionan con el burro.

La prueba reproducible más reciente registró un aumento de 33 a 80 puntos de vínculo y el cambio de nivel 0 a 1. La mula y la mula pintada están incluidas en la lista de modelos admitidos, pero V0.1 no afirma que todos sus casos de vínculo hayan sido probados de forma equivalente.

### Limitaciones conocidas

- En el burro, **Alimentar**, **Cepillar** y **Acariciar** todavía no están habilitados en el menú contextual a pie. Alimentar montado y cepillar usando el objeto del inventario sí funcionaron en la prueba.
- Algunas vistas pueden mostrar el nombre o las estadísticas de Tennessee Walker. Es un efecto colateral conocido de la adaptación temporal de modelo.
- Guardar, recuperar o renombrar burros y mulas en el establo todavía no está implementado. El guardado vanilla de mulas puede alterar los espacios del establo; no uses el establo con estos animales esperando persistencia de V0.1.
- La compatibilidad con Stable Expansion aún no está implementada.

V0.1 se centra en el avance comprobado del bonding. Las opciones contextuales a pie y el manejo del establo quedan para versiones posteriores.

## Requisitos

- Red Dead Redemption 2 **Story Mode**, Steam, versión probada **1.0.1491.50**.
- Windows 10/11.
- ScriptHookRDR2 y un ASI loader funcional.
- Rampage Trainer no es un requisito del mod; se usó durante las pruebas.

**No usar en Red Dead Online.** El hook de bonding depende de una dirección interna y una firma de instrucciones del ejecutable. Si la firma no coincide, el mod se niega a instalar ese hook; otras versiones del juego no están verificadas.

## Instalación

1. Descarga `StabledDonkey.asi` desde **Assets** de la release `v0.1.0`.
2. Copia el archivo al directorio raíz de RDR2, junto a `RDR2.exe`.
3. Inicia el juego en Story Mode. El mod crea `StabledDonkey.log` en el directorio de trabajo del juego.

ScriptHookRDR2 es un requisito externo y debe descargarse desde [la página oficial de Script Hook RDR2](http://www.dev-c.com/rdr2/scripthookrdr2/). Este repositorio no redistribuye el SDK ni `ScriptHookRDR2.dll`.

## Compilar desde el código fuente

Requisitos de desarrollo: CMake 3.20 o posterior, Visual Studio Build Tools con MSVC y el SDK de ScriptHookRDR2 instalado localmente.

Descarga el SDK desde la página oficial indicada arriba y extrae su contenido. Desde PowerShell, configura la ruta local al SDK y compila:

```powershell
cmake -S . -B build -DSCRIPTHOOK_RDR2_SDK="C:/ruta/al/ScriptHookRDR2"
cmake --build build --config Release
```

El resultado es `build/Release/StabledDonkey.asi`. Si el SDK está en `sdk/ScriptHookRDR2`, se detecta automáticamente. Esa carpeta está excluida de Git para cumplir las condiciones de redistribución del SDK.

## Modelos admitidos

| Animal | Modelo | Hash |
|---|---|---:|
| Burro | `A_C_Donkey_01` | `0x69A37A7B` |
| Mula | `A_C_HorseMule_01` | `0xB6A7CE35` |
| Mula pintada | `A_C_HorseMulePainted_01` | `0x23685521` |

## Registro de diagnóstico

`StabledDonkey.log` registra la instalación del hook, el modelo de la montura y sus puntos/nivel de vínculo observados. Para informar un problema, adjunta el log creado durante esa sesión y especifica el modelo probado y la acción realizada.

## Licencia

Este proyecto aún no incluye una licencia de reutilización. Todos los derechos del código fuente quedan reservados por su autor mientras no se publique una licencia explícita. Los componentes externos conservan sus propias condiciones.
