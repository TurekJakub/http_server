# Tvorba vlastních HTTP handlerů

Následující kapitola pojednáva o přidání vlastní logiky obsluhy příchozích požadavků při knihovním využití tohoto projektu. Než přejdeme k samotné tvorbě vlastních HTTP handlerů a jejich registraci ze strany knihovního `HttpServer` objektu bylo by vhodné nastínit obecný mechanismus obsluhy příchozích požadavků, protože poskytuje hlubší vhled do řešené problematiky a některé jeho aspekty mohou být užitečné i při tvorbě vlastních handlerů.

## Obsluha příchozích požadavků

Abstrakce doposud vybudovaná v předchozích kapitolách nás zanechala s možnostmi efektivně navázat spojení s klientem a následně prostřednictvím tohoto spojení přijmout příchozí HTTP požadavek. Posledním krokem k plně funkčnímu HTTP serveru je schopnost na základě tohoto požadavku vygenerovat odpovídající HTTP odpověď a navrátit jí klientovi.

Prvním krokem k dosažení této funkcionality je routing. V tomto kroku server jednouše předá přijatý požadavek dedikované router komponentě, která si následně pouze přečte target tohoto požadavku a pokusí se ho nalézt ve své routovací tabulce, pokud nenalezne vyhovující záznam je využit defaultní handler, který typicky pouze vrátí HTTP 404 Page not found odpověď. Samotná routovací tabulka je pouze jednoduchým mapováním jednotlivých targetů reprezentovaných textovým řetězcem na konkrétní volatelné objekty představující koncové handlery. Následně router vytvoří prázdný objekt HTTP odpovědi a zavolá nalezený handler, kterému předá reference na objekt požadavku, který obdržel od serveru a jím vytvořený objekt odpovědi. Následně router pouze zkontroluje výsledek obsloužení požadavku v podobě návratové hodnoty handleru (handler může oznámit chybu routeru pomocí návratové hodnoty v podobě `std::expected<void, HandlerError>`). Pokud handler v tomto kroku signalizoval chybu navrácením `HandlerError` objektu, provede router automatické zpracování této chyby, to spočívá v zalogování chybové zprávy navrácené uvnitř chybového objektu (v rámci chybové odpovědi jsou klientovi odesílany pouze generické HTTP chyby, nedochází tedy k úniku potenciálně znužitelných informací mimo server) a navrácení HTTP odpovědi s jednoduchou chybovou HTML stránkou (HTTP status zobrazený klientovi se opět odvýjí od návratové hodnoty handleru), která je předána serveru jako výsledek obsloužení a odeslána klientovi. Stejný mechanismus router použije v případě, že při volání handleru došlo k vyjímce. Pokud volání handleru proběhlo úspěšně, router jednoduše vrátí handlerem zapsaný objekt odpovědi serveru k odeslání.

Nakonec této obecné sekce se ještě podrobněji podívejme na podobu samotných handlerů. Obecně je jediným nárokem kladeným na HTTP handler nutnost splňovat následující koncept.

``` c++
// HTTP handler concept
template <typename T>
concept handler = requires(T handler_func, const HttpRequest &req, HttpResponse &resp) {
  { handler_func(req, resp) } -> std::same_as<void>;
} || requires(T handler_func, const HttpRequest &req, HttpResponse &resp) {
  { handler_func(req, resp) } -> std::same_as<std::expected<void, HandlerError>>;
};
```

Tedy být volatelným, přijmat argumenty v podobě const reference na HttpRequest a reference na HttpResponse a vracet buďto návratovou hodnotu pro reportování chyby uvedenou výše, nebo žádnou hodnotu. Varinta bez návratové hodnoty slouží pouze pro zjednodušení rozhraní jednoduchých handlerů, které nemají potřebu selhávat (například defaultní handler, který pouze vrácí jednoduchou `404 Page not found` odpověď a podobně) a interně jsou i tyto handlery wrapovány tak, aby možné s nimi snadno pracovat pomocí jednotného rozhraní. Týmto finálním jednotným rozhraním je pak `std::move_only_function<std::expected<void, HandlerError>(const HttpRequest &, HttpResponse &)>`, které slouží primárně jako type erasure pro snadnou manipulaci s handlery v rámci routeru.

## Vlastní HTTP handlery
