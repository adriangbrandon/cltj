---
name: compact-ds
description: Convenciones para implementar, probar y medir estructuras de datos compactas en C++ (bitvectors, rank/select, wavelet trees, k2-trees, tries comprimidos). Úsala siempre que escribas o revises código de este repo.
---

# Estructuras de datos compactas — convenciones del repo

> Borrador. Ajústalo a tu estilo (y a sdsl si la usas) antes de dárselo a los agentes.

## Código
- C++17 salvo que el repo indique otra cosa. Nada de coroutines, modules ni ranges en código caliente.
- Palabra de 64 bits (`uint64_t`). Bits indexados desde 0, LSB primero dentro de cada palabra.
- Operaciones de bits con `__builtin_popcountll`, `__builtin_ctzll`, `__builtin_clzll`.
  Comprueba siempre el caso de argumento 0 en ctz/clz (comportamiento indefinido).
- Desplazamientos: nunca `x << 64` ni `x >> 64` (UB). Usa máscaras explícitas para 0 y 64 bits.
- Lecturas/escrituras de campos de `w` bits que crucen frontera de palabra: función única y probada
  (`get_bits(pos, w)` / `set_bits(pos, w, v)`), no código repetido.
- Sin asignaciones dinámicas en consultas. Estructuras inmutables tras la construcción salvo que la
  especificación diga lo contrario.
- Cada estructura expone `size_in_bytes()` que cuente TODO: datos, muestreos, tablas y punteros.
- Serialización `serialize(std::ostream&)` / `load(std::istream&)` cuando la estructura sea persistente.

## Tests
- Implementación naive de referencia en `tests/naive/` (vector<bool>, vector<uint64_t>, etc.).
- Property tests con semilla fija y registrada: comparar cada operación contra la naive sobre
  entradas aleatorias de varias densidades (1%, 10%, 50%, 90%, 99%).
- Casos límite obligatorios: n=0, n=1, n=63, 64, 65, potencias de 2 ± 1, todo ceros, todo unos,
  último bloque/superbloque parcial, rank(n), select del último 1.
- Todos los tests pasan con `-fsanitize=address,undefined`.

## Benchmarks (`bench/run.sh`)
- Compilación de bench separada: `-O3 -march=native -DNDEBUG`, SIN sanitizers.
- Consultas generadas con semilla fija; mismas consultas para todas las variantes.
- Calentamiento y luego mediana de al menos 5 repeticiones. Reporta ns/consulta.
- Espacio en bits/elemento usando `size_in_bytes()` (incluye sobrecoste) y también el desglose.
- Usa un acumulador sobre los resultados (y imprímelo) para que el compilador no elimine las consultas.
- Salida en tabla: `estructura | n | densidad | bits/elem | ns/op`.

## Revisión: qué mirar primero
1. Fronteras de palabra y de bloque.
2. Desbordamientos en índices (`uint32_t` vs `uint64_t` para posiciones con n grande).
3. Que `size_in_bytes()` no omita estructuras auxiliares.
4. Que la complejidad real coincide con la de la nota del paper (sin bucles lineales ocultos).
