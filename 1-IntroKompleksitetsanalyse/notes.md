# Intro

Python ok i starten, ikke anbefalt for senere større øvinger...

## Kompleksitetsanalyse

Asymptotisk analyse - når datamengden går mot uendelig.
Teller bare løkker og metodekall, kun interessert i største eksponent

## O-notasjon

det finst konstanser n0 og c slik at:
0 <= f(n) <= c \* g(n) for alle n >= n0
konstantene velger vi selv

### øvre grense

#### eks1

f(n) = n^2 - 2n + 4 endelav O(n^2)

vi setter inn f(n) i definisjonen og prøver:
0 <= f(n) <= c \* n^2

del på n^2:
0 <= 1 - 2/n + 4/n^2 <= c

fungerer for n0 = 10 og c = 2

#### eks2

n^2 - 2n + 4 ikkedelav O(n)
setter inn f(n) i definisjonen og prøver
0 <= n^2 - 2n + 4 <= c \* n for alle n >= n0
0 <= n - 2 + 4/n <= c

### nedre grense

omega i stedet for O
omega, den nedre grensen, "ikke bedre enn dette"

f(n) endelav omega(g(n)) => g(n) er en nedre grense for f(n)

0 <= c \* g(n) <= f(n) for n >= n0

#### eks3

f(n) = 2n^3 - n - 7 endelav omega(n^3)
setter inn f(n) i definisjonen og prøver:
0 <= c \* n^3 <= 2n^3 - n - 7
0 <= c <= 2 - 1/n^2 - 7/n^3

gjelder for n0 = 10 og c = 1

### theta(n), felles øvre og nedre grense

brukes når O og omega har samme uttrykk

def: f(n) endelav theta(g(n)) hvis det fins konstanser c1, c2 og n0 så:
0 <= c1 _ g(n) <= f(n) <= c2 _ g(n) for n >= n

### vanlige kompleksiter

O(1), O(n), O(n^2), O(n^3), O(n^4), O(n^5),...
O(log n), O(n log n), O(2^n)
Korte og greie uttrykk som er lette å sammenligne
Kan finne ut hvilke programmer som er raskest på store datamengder

## måle effektivitet - måleteknikk

utskrift kan innebære millioner av operasjoner, kan forstyrre målinger med feks n = 1000
gjør alle målinger og så gjør utskrift

annen programvare kan forstyrre
maskinklokka har ikke høy nok oppløsing for svært korte kjøringer

vanlig problem hvis datasettet muteres (sortering senere):
andre runde går raskere enn første osv
må lage kopi av datasettet hver gang

ved objektorientert programmering:
ikke gi klassen navn etter algoritmen
algoritmer implementeres som metoder
klasser får navn etter hva de inneholder av data
