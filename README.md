# Zadanie 12.9 - Planowanie gospodarcze (Williams)

Model LP rozwiązany w CPLEX Concert Technology (C++).

## Uruchomienie

```bash
./economic_planning [1|2|3]
```

Argument wybiera cel optymalizacji:
1. Maksymalna zdolność produkcyjna na koniec roku 5
2. Maksymalna produkcja w latach 4-5
3. Maksymalne zatrudnienie (bez limitu manpower)

## Wyniki

| Cel | Wartość funkcji celu |
|-----|---------------------|
| 1   | 2306.64             |
| 2   | 2618.58             |
| 3   | 2068.75             |

## Struktura modelu

- **Zmienne:** produkcja `x[i][t]`, rozbudowa `y[i][t]`, zapasy `s[i][t]`
- **Ograniczenia:** bilans materiałowy, limity zdolności, limit manpower
- **Horyzont:** 5 lat (rok 0 = stan początkowy)