# ZADÁNÍ PROJEKTU: Optimalizace vizualizace vulkanické činnosti pomocí SIMD


Cílem projektu je optimalizovat výpočetně náročné algoritmy pro vizualizaci činnosti sopky pomocí vektorových instrukcí (SIMD/AVX). Projekt je rozdělen do dvou hlavních částí: jedna řeší dynamiku vytékající lávy (včetně šíření tepla) a druhá fyziku částic vystupujících ze sopky (vulkanický popel a materiál). Můžete použít SSE, AVX a AVX2. Nezapomínejte, že je zapotřebí myslet i na skalární zpracování prvků, které se vám nevlezou do registru.

## Prostředí a implementace
Projekt můžete řešit jak na operačním systému Linux, tak i pro Windows (zde si dejte velký pozor na rozdíly ve volacích konvencích / Calling Conventions). Stejně jako na cvičeních budete pracovat s čistým GAS asemblerem v souboru volcano_kernels.s, kde jsou připravené signatury funkcí, které budete implementovat.

V aplikaci se pro referenci a kontrolu defaultně volá výchozí skalární implementace v jazyce C++, kterou naleznete v souboru VolcanoSim.cpp. Veškerá data o částicích a mřížce jsou navržena ve formátu SoA (Structure of Arrays) s ohledem na snadnější vektorizaci a zarovnána do paměti.

## Testování, validace a benchmark
V případě, že chcete provádět benchmark a nebo získat referenční hodnoty pouze pro algoritmy vulkánu (bez nutnosti spouštět kompletní grafické rozhraní), můžete při spuštění programu z příkazové řádky použít následující přepínače:

--volcano-reference [počet_framů]
Tento režim spustí simulaci pro zadaný počet framů (výchozí hodnota je 10). Během běhu nejprve vypisuje referenční hodnoty skalární simulace (např. průměrnou výšku lávy, pozice a teplotu částic), což vám poslouží jako vzor pro ladění. 

--volcano-benchmark
Spustí výkonnostní test pro pevně daný počet kroků. Aplikace změří celkový čas běhu (v milisekundách) samostatně pro skalární a vaši SIMD cestu. Výstupem bude naměřený čas obou verzí a výsledný Speedup faktor (poměr zrychlení), který bude hrát klíčovou roli v hodnocení optimalizace.

## Doporučený postup řešení
Optimalizaci doporučuji provádět postupně na základě zdrojového kódu v C++ (VolcanoSim.cpp). Před samotným psaním kódu v asembleru si důkladně analyzujte klíčové části výpočtů (zejména matematické smyčky uvnitř funkcí) a ujasněte si, jaké datové struktury se předávají a jak chcete paměť mapovat do SIMD registrů (např. YMM).

## Hodnocení a podmínky získání bodů
Vzhledem k tomu, že je v dnešní době prakticky nemožné kontrolovat, kdo z vás použil při generování kódu umělou inteligenci a do jaké míry, je zisk bodů z projektu striktně podmíněn osobní obhajobou.

Na obhajobě byste měli být schopni prokázat, že vašemu řešení plně rozumíte, víte, co která část vámi odevzdaného kódu dělá, a stejně tak umíte detailně vysvětlit, jak fungují vámi použité asemblerové instrukce.


## Algoritmy pro implementaci

---

### 1. Dynamika částic (`updateParticlesSIMD`)

Tento algoritmus počítá pohyb, chladnutí a životnost částeček popela a materiálu vyvržených ze sopky. Pro každý krok `dt` a částici `i` s pozicí $(x_i, y_i, z_i)$, rychlostí $(v_{xi}, v_{yi}, v_{zi})$ a teplotou $T_i$ se provedou následující výpočty:

**A. Pomocné proměnné a normalizace:**
Nejprve se zjistí vzdálenost částice od středu sopky (jícnu) a normalizuje se její teplota.

* **Vzdálenost a inverzní poloměr:** $r^2 = x_i^2 + z_i^2$, $invR = \frac{1}{\sqrt{\max(r^2, \epsilon_r)}}$
* **Normalizovaná teplota:** $T_{norm} = \text{clamp}\left(\frac{T_i - T_{amb}}{T_{span}}, 0, T_{normMax}\right)$

**B. Výpočet zrychlení (sil působících na částici):**

* **Osa Y (Gravitace a vztlak):** Horké částice stoupají vzhůru, ale vztlak slábne s výškou.

$$a_y = g + B \cdot T_{norm} \cdot \frac{1}{1 + k_b \cdot \max(y_i, 0)}$$


* **Osy X a Z (Vítr a radiální síla jícnu):** Jícen částice odtlačuje do stran a do toho fouká vítr, jehož vliv závisí na teplotě částice.

$$a_{vent} = \max(0, R_{vent}^2 - r^2) \cdot K_{vent} \cdot T_{norm}$$


$$a_x = windX \cdot (c_{w0} + c_{w1} \cdot T_{norm}) + x_i \cdot invR \cdot a_{vent}$$


$$a_z = windZ \cdot (c_{w0} + c_{w1} \cdot T_{norm}) + z_i \cdot invR \cdot a_{vent}$$



**C. Integrace pohybu (Euler) a tlumení odporu prostředí:**

* Aktualizace rychlosti o zrychlení: $\vec{v}_i = \vec{v}_i + \vec{a}_i \cdot dt$
* Výpočet lokálního tlumení (odporu vzduchu): $d_{local} = \text{clamp}(d_0 - d_1 \cdot T_{norm}, d_{min}, d_{max})$
* Aplikace tlumení na rychlost: $\vec{v}_i = \vec{v}_i \cdot d_{local}$
* Posun pozice: $\vec{p}_i = \vec{p}_i + \vec{v}_i \cdot dt$

**D. Životnost a chladnutí:**

* Úbytek života: $life_i = life_i - dt \cdot (c_{l0} + c_{l1} \cdot T_{norm})$
* Ztráta tepla: $T_i = T_i - k_{cool} \cdot dt \cdot (c_{t0} + c_{t1} \cdot \sqrt{r^2 + \epsilon_t})$

*(Pozn.: Kolize s terénem se po provedení tohoto SIMD bloku řeší ve skalární smyčce, to v assembleru pravděpodobně dělat nebudete, resp. závisí na přesném zadání struktury souboru).*

---

### 2. Tok lávy na mřížce (`updateLavaFluxSIMD`)

Láva se šíří po 2D mřížce jako buněčný automat. Tok probíhá vždy mezi středovou buňkou $C$ a jejími čtyřmi sousedy (Left, Right, Up, Down).

**A. Fluidita lávy:**
Láva teče tím lépe, čím je teplejší.

* $f_C = \text{clamp}\left(\frac{T_C - T_{liq}}{T_{liqSpan}}, 0, 1\right)$
* Koeficient toku: $k_C = viscosity \cdot dt \cdot (k_0 + k_1 \cdot f_C)$

**B. Výpočet toku do okolí (funkce Flow):**
Nejprve se spočítá celková výška v buňce ($H_C = terrain_C + lava_C$) a v sousední buňce (např. $H_L$). Poté se určí "spád" (drop): $drop_{C \to L} = H_C - H_L$.

* Samotný tok z $C$ do $L$ počítá nelineární funkce (teče to jen z kopce, tj. pro kladný spád):

$$out_L = \text{flow}(drop, k_C) = \max(0, drop) \cdot k_C \cdot \left(a_0 + a_1 \cdot \sqrt{\max(0, drop) + \epsilon}\right)$$



Tímto způsobem se vypočítá tok do všech 4 směrů: $out_L, out_R, out_U, out_D$.

**C. Ochrana proti "vysátí" buňky:**
Nelze nechat odtéct více lávy, než v buňce fyzicky je (a navíc je zde limitující konstanta $\alpha_{out}$).

* Suma odtoku: $outSum = out_L + out_R + out_U + out_D$
* Maximální povolený odtok: $maxOut = lava_C \cdot \alpha_{out}$
* Pokud by chtělo odtéct víc, toky se zmenší: Je-li $outSum > maxOut$, pak každý směr vynásobíme zlomkem $\frac{maxOut}{outSum}$.

**D. Aktualizace stavu buňky:**
Spočítají se celkové přítoky ze sousedů ($inSum$) a zapíše se nová hodnota výšky lávy.

* $lavaNext_C = \text{clamp}(lava_C + inSum - outSum, 0, lavaMax)$

---

### 3. Difuze teploty (`diffuseHeatSIMD`)

Teplota se po mřížce šíří vedením tepla, ale je také generována přítomností horké lávy a naopak se ztrácí ochlazováním do okolního prostředí.

**A. Vedení tepla (Laplaceův operátor):**
Zhodnocení rozdílu teploty vůči 4 přímým sousedům.

* $lap = T_L + T_R + T_U + T_D - 4 \cdot T_C$

**B. Zdroje a ztráty tepla:**

* Fluidita (stejný vzorec jako u lávy): $f = \text{clamp}\left(\frac{T_C - T_{liq}}{T_{liqSpan}}, 0, 1\right)$
* Ohřev od lávy přítomné v buňce: $Q_{lava} = lava_C \cdot (q_0 + q_1 \cdot f)$
* Ochlazování do okolí: $Q_{cool} = k_{loss} \cdot (T_C - T_{amb})$

**C. Integrace a aktualizace (Euler):**

* Výpočet nové teploty: $T_{next} = T_C + dt \cdot (\alpha \cdot lap + Q_{lava} - Q_{cool})$
* Zamezení nesmyslných teplot (clamp): $T_{next} = \text{clamp}(T_{next}, T_{amb}, T_{max})$

---

### Algoritmický postup implementace (pro SIMD AVX2)

Až budete psát asembler (`.s` soubor), postup pro každý výše zmíněný Kernel bude zhruba následující:

1. **Vstupní parametry:** Ukazatele na pole (`posX`, `velX`, `lavaHeight`, atd.) si namapujete do 64bitových registrů (např. `rdi`, `rsi` u Linuxu / `rcx`, `rdx` u Windows). Ostatní konstanty (jako $dt$, $g$, $k_b$) si načtete pomocí `vbroadcastss` do YMM registrů.
2. **Smyčka po 8 prvcích (YMM registry pojmou 8x 32-bit float):**
* Pomocí instrukcí `vmovups` (příp. `vmovaps` pro zarovnaná pole) načtete 8 prvků dané vlastnosti.
* Běžné operace přeložíte na `vaddps`, `vmulps`, `vsubps`.
* Funkce `clamp(x, min, max)` nebo `max(0, x)` zkonstruujete jednoduše složením instrukcí `vmaxps` a `vminps`.
* Matematické funkce provedete přes `vsqrtps` a inverzní dělení (`1 / x`) ideálně přes `vrcpps` (nebo `vrsqrtps` pro $1/\sqrt{x}$).


3. **Zápis:** Spočítané hodnoty opět zapíšete do příslušných indexů pomocí `vmovups`.
4. **Tail processing:** Pokud celkový počet prvků není dělitelný 8, zbytek se na konci spočítá po jedné (skalárně). U částic se dělá skalární smyčka s `XMM` registry nebo volání C++ fallbacku.
