# light_md

Editor de Markdown ligero en C++ y Qt 6, pensado para personas que no son del área de tecnología.

Escribe a la izquierda y lee el resultado, ya bonito, a la derecha. No hace falta saber Markdown: los botones de arriba dan el formato por ti.

## Qué hace

- Dos paneles: escritura a la izquierda, lectura a la derecha, con vista previa al instante.
- Botones con palabras de todos los días: Nuevo, Abrir, Guardar, Título grande, Negrita, Lista con puntos, Cita, Enlace, etc.
- Abrir y guardar documentos `.md`.
- Contador de palabras y aviso de cambios sin guardar.

## Compilar (Windows, con Qt 6 y MinGW)

Necesitas Qt 6 instalado (por ejemplo en `C:\Qt`) con su CMake y Ninja.

```powershell
$env:PATH = "C:\Qt\Tools\mingw1510_64\bin;C:\Qt\6.12.1\mingw_64\bin;$env:PATH"
C:\Qt\Tools\CMake_64\bin\cmake.exe -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.12.1/mingw_64
C:\Qt\Tools\CMake_64\bin\cmake.exe --build build
.\build\light_md.exe
```

Ajusta las rutas si tu versión de Qt o de MinGW es otra.

## Licencia

MIT
