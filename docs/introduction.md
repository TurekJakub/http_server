# Úvod

Tento projekt vznik jako zápočtový projekt v rámci předmětu Programování v C++ v zimním semestru akademického roku 2025/26 na Matematicko-fyzikální fakultě Karlovy univerzity, proto níže uvádím znění jeho schváleného zadání.

## Zadání projetu

Cílem projektu je implementovat jednoduchý HTTP server, který bude umožňovat paralerní zpracování požadavků za využití multithreadingu. Server by dále měl podporovat zabezpečení komunikace s klientem pomocí TLS protokolu.

Projekt by zároveň měl podporovat dva základní režimy fungování. Jednak by měl fungovat jako klasický web server, který bude možné nakonfigurovat pomocí konfiguračního souboru ve formátu TOML a po spuštění bude přes HTTP sevovat staické soubory z filesystemu hostitelského systému dle této konfigurace.

Druhým režimem pak bude režim knihovny, kdy bude projekt poskytovat základní framework pro síťovou komunikaci prostřednictvím HTTP. Uživatel knihovny pak bude schopen ze svého kódu jednoduše vytvořit instanci HTTP serveru a pro konkrétní endpointy zaregistrovat akce (handlery), které server následně použije k obsuluze příchozích HTTP požadavků pro daný endpoit.

Při implementaci projektu pak budou využity knihovny ASIO a OpenSSL poskytující implementaci multiplatformních TCP socketů a TLS protokolu.

## Úvod do řešené problematiky

Při implementaci projektu toho projektu bylo třeba vyřešit několik klíčových problému, které ve stručnosti nastiňuje tato podkapitla a následně budou podrobněji diskutovány v následujících kapitolách.

Prvním z problému, který je třeba vyřešit při implemaci témněř libovolné serverové aplikace, je návrh modelu síťové komunikace, tedy způsob navazování spojení s klientem a následná obsluha tohoto spojení. V případě tohoto projektu je s tímto problémem úzce spjat i problém paralelizace a konkurenčního zpracování navazovaných spojení.

Vzhledem k tomu, že implementovaný projekt je HTTP serverem s podporou pro HTTPS, je dalším klíčovým problémem projektu dostatečná implementace práce s tímto projektem, která umožní serveru s ktlientem komunikovat prostřednictvým dříve navázaných spojení.

Posledním z klíčových problému implementace je pak navržení samotného frameworku a veřejného knihovního rozhraní, které uživately knihovny umožní práci s HTTP protokolem a tvorbu vlastního HTTP servru. Toto rozhraní tak musí být dostatečně flexibilní, aby uživately umožnilo efektivní práci s HTTP spojením při tvorbě vlastních handlerů, a zároveň poskytovat dostatečnou abstrakci, která zapouzdří výše zmíňěnou nízkoúrovňovou síťovou komunikaci pomocí TCP socketů, navazování samotného HTTPS spojení a konkurentního zpracování příchozích požadavků. Využití tohoto rozhraní je pak demonstrováno při implementaci samotného web serveru, který je implementován právě prostřednictvím frameworku poskytovaného knihovní částí projektu.

Nyní již přejděme k popisu navazovaní a obsluhy spojení, o kterém pojednává následující kapitola.

[<-- Obsah](index.md) <div style="margin-left:auto; margin-right:0px">[Obsluha příchozích spojení -->](network_connections.md)</div></div>
