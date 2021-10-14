**<p align="center"> Systemy operacyjne, Symulacja Baru  </p>**
_________________________________
**<p align="center"> Politechnika Wrocławska </p>**
<p align="center"> Maciej Jaroński </p>


<a name="desc"></a>
# Opis aplikacji
Aplikacja ma na celu symulacje obsługi klientów w barze w którym serwowane jest piwo.
Klienci przybywają do baru. W nim zajmują miejsca przy stolikach (stoliki nie są
uwzględnione w projekcie, należy założyć że klienci zwyczajnie gdzieś przebywają w barze).
Każdemu klientowi podczas pobytu w barze, z czasem rośnie stan pragnienia. Stan ten nie
może nigdy spaść do zera. Klienci uzupełniają pragnienie ustawiając się w kolejki po piwo,
nalewane przez barmanów.


## Założenia

• Klient chcący zaspokoić pragnienie musi podejść do barmana.

• Barmanów jest kilka, jednak znacznie mniej niż klientów

• Gdy klientów chcących zamówić piwo jest więcej niż wolnych barmanów ustawiają
się oni w jedną kolejkę do baru.

• Gdy klient uzyskał dostęp do baru, zamawia piwo. Z nim odchodzi z baru i zwalnia
miejsce w kolejce do barmana

• Klient z piwem wypija je przez pewien czas. Po wypiciu zostawia na barze brudny
kufel.

• Barmani mają ograniczoną ilość kufli do nalewania piwa. W przypadku gdy brak jest
czystych kufli, są nie zdolni do nalewania piwa. W takim wypadku biorą pewną ilość
kufli i udają się by je umyć

• Barmani myją kufle w zmywakach, których ilość również jest ograniczona (mniej niż
barmanów). W przypadku gdy wszystkie zmywaki są zajęte – barmani chcący umyć
kufle ustawiają się w kolejkę.

• Zasób czystych kufli jest dostępny dla każdego barmana. Oznacza to że wszystkie
czyste kufle są przechowywane na jednym stosie do którego dostęp ma każdy
barman.

• Klientom wraz z piciem kolejnych piw zwiększa się potrzeba skorzystania z toalety. Gdy wzrośnie do 100% klient idzie skorzystać z toalety.

• Jeśli toaleta jest wolna, klient chcący z niej skorzystać zajmuje ją na określony czas.

• Gdy brak wolnych toalet, klienci chcący z niej skorzystać ustawiają się do niej w kolejkę.


## Wizualizacja

[Demo symulacji](https://youtu.be/-LpcvREs5n0)

# Stos technologiczny
- LINUX
- GCC
- C++
-ncurses
