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

Nyní se můžeme přesunout k samotnému využití knihovního rozhraní projektu. Následujíci kapitola se tedy bude primárně praktickou úkázkou implementace vlastního HTTP serveru s využitím poskytovaného knihovního rozhraní. Samotné knihovní rozhraní není příliš složité, jak již bylo zmíně dřive, většína obecné logiky serveru je zapouzdřena uvnitř `HttpServer` objektu, který knihovna poskytuje v rámci `http_server::http` namespace v headeru `server`. Při tvorbě vlastního serveru tedy začneme naincludováním tohoto headru a vytvořením instance serverového objektu. `HttpServer` pro svou inicializaci vyžaduje poskytnutí základní konfigurace předávané v podobě `HttpConfig` objektu. V rámci této konfigurace je nutné serveru předat port, na němž má začít poslouchat, a teké cesty k souborům obsahujícím SSL certifikát a k němů náležící privátní klíč, jelikož server v současné chvíli vynucuje zabezpečení komunikace pomocí TLS (pokud je poskytnutý klíč zašifrován server předpokládá předání příslušné passphrase prostřednictvím systémové proměné `PRIVATE_KEY_PASS`, nebo případně zažádá o její zadání do konzole po svém spuštění). Volitelně je také možné specifikovat maximální počet vláken v interním thread poolu server (při neuvedení je automaticky nastavena na počet hardwarových vláken dle `std::thread::hardware_concurrency()`). Ten sám o sobě představuje plně funkční HTTP server, který ale postrádá jakoukoliv logiku pro obsluhu příchozích požadavků. Tu je takto vytvořenému serveru třeba dodat registrací již zmíněných HTTP handlerů.

Jak již bylo zmíněno jediným požadavkem na HTTP handler je splnění výše uvedeného `handler` conceptu. V praxi jím většinou bude lamba, nebo funktor (funktory jsou ideální pro tvorbu složitějších handlerů vzhledem k snadnému uchování vnitřního stavu), případně named funkce. Pro samotné zaregistrování handleru k obsluze konkrétního endpointu poskytuje server rozhraní v podobě metody `add_handler(string route, T &&handler_func, bool prefix_match)`, která zaregistruje handler předaný jako `handler_func`, k obsluze endpointu odpovídajícímu cestě `route`. Volitelný argument `prefix_match` slouží ke specifikaci provnávání cest routerem při routování požadavků, defaultně je tento argument nastaven na `false` a router tedy pro tento endpoint používá klasické porovnání na rovnost (tedy target požadavku se musí přesně shodovat s `route` uvedenou při registraci handleru, aby došlo k jeho přesměrování na daný endpoint/handler). Pokud je však `prefix_match` při registraci handleru nastaven na `true`, pak pro přesměrování požadavku na daný endpoint stačí, aby jeho target obsahoval uvedený `route` jako prefix. Pokud bychom tedy například zaregistrovali handler s názvem `static_handler` jako prefix match handler na `route` `/static`, tak na něj bude přesměrován jak požadavek na `/static/index.html`, tak požadavek na  `/static/assets/favicon.svg`. V případě, že by danému targetu vyhovovalo více ze zaregistrovaných handlerů, router vždy upřednostní přímou shodu před shodou v prefixu (tedy handler s `prefix_match` nastaveným na `false`). Pokud dojde při přesměrovávíní požadavku ke shodě s více handlery a všechny používají prefixové porovnání, pak záleží na lexikografickém pořadí jednotlivých `route`, které určí, který z handlerů bude vybrán (obecně je vhodné užívat prefixové porovnání uvážlivě a vhýbat se kolizím mezi handlery). Opakované volání `add_handler` se stejným `route` argumentem povede k přepsání předchozího záznamů (vždy je tedy používán pouze poslední handler zaregistrovaný pro daný route). Tímto způsobem tedy lze serveru dodat požadovanou logiku pro zpracování příchozích požadavků. Užitečné může být také specifikovat, jak má server reagovat na požadavky, pro něž nenalezne vyhovující handler. K tomuto účelu slouží rozhraní serveru v podobě metody `set_default_handler(T &&default_handler)`, která je obdobou již zmíněného `add_handler` s tím rozdílem, že na takto nastavený handler budou přesměrovány všechny požadavky, pro nějž router nenalezne vyhovující handler.

Následující úkazka kódu pak demonstruje využití knihovny k tvorbě minimalistického HTTP serveru dle popisu výše. Rozsáhlejší ukázku užití v podobě jednoduchého web serveru pak lze nalázt v podadresáři `src/http_server/`. Při implementaci vlastních HTTP handlerů pak mohou být užitečné také některé utility nacházející se v `http_server::http` namespace v `http` headeru.

``` c++
#include <expected>
#include <format>
#include <string>

#include <http_server/server.h>

using namespace std;
using namespace http_server::http;

struct BasicHandler {
public:
  BasicHandler(string message) : message(message) {};
  expected<void, HandlerError> operator()(const HttpRequest &req, HttpResponse &res) const {
    // refuse all non GET request - HandlerError get automatically wrapped as HTTP 405 response
    if (req.method != GET) {
      return unexpected(HandlerError("Only GET requests are allowed", HttpStatus::MethodNotAllowed));
    }

    // set response status
    res.status() = HttpStatus::OK;
    // set some response headers
    res.headers().set("Content-Type", "text/html");
    // fill response body with simple html
    string body_content = format("<!DOCTYPE html><html><body><h1>{}</h1></body></html>", message);
    res.body() = HttpBody{body_content.begin(), body_content.end()};

    return {};
  }

private:
  string message{""};
};


int main() {
  // create server instance with TLS certificate listening on port 8080
  HttpServer server{
      {8080, "../resources/secret/cert.pem", "../resources/secret/key.pem"}
  };

  // create few basic handlers
  BasicHandler prefix_handler {"Hello from prefix handler!"};
  BasicHandler root_handler {"Hello from standard root handler!"};
  BasicHandler default_handler {"Hello from default handler!"};

  // register handlers
  server.add_handler("/prefix", prefix_handler, true);
  server.add_handler("/", root_handler);
  server.set_default_handler(default_handler);
  
  // start serving incoming request
  server.start();

  return 0;
}
```

Tímto bychom uzavřeli kapitolu týkající se využití knihovního rozhraní projektu a implementace vlastních HTTP handlerů. A přesunuli se k závěrečné kapitole pojednávající o sestavení projektu a jeho konfiguraci.

[<-- implementace HTTP protokolu](http.md) <div style="margin-left:auto; margin-right:0px ">[Konfigurace a použití -->](configuration_and_usage.md)</div></div>
