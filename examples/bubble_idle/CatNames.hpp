#pragma once

#include <algorithm>
#include <string>
#include <vector>


////////////////////////////////////////////////////////////
[[nodiscard]] std::vector<std::string> getShuffledCatNames(auto&& randomEngine)
{
    std::vector<std::string>
        names{"Gorgonzola", "Provolino", "Pistacchietto", "Ricottina",     "Mozzarellina", "Tiramisu",   "Cannolino",
              "Biscottino", "Cannolina", "Biscottina",    "Pistacchietta", "Provolina",    "Arancino",   "Limoncello",
              "Ciabatta",   "Focaccina", "Amaretto",      "Pallino",       "Birillo",      "Trottola",   "Baffo",
              "Poldo",      "Fuffi",     "Birba",         "Ciccio",        "Pippo",        "Tappo",      "Briciola",
              "Braciola",   "Pulce",     "Dante",         "Bolla",         "Fragolina",    "Luppolo",    "Sirena",
              "Polvere",    "Stellina",  "Lunetta",       "Briciolo",      "Fiammetta",    "Nuvoletta",  "Scintilla",
              "Piuma",      "Fulmine",   "Arcobaleno",    "Stelluccia",    "Lucciola",     "Pepita",     "Fiocco",
              "Girandola",  "Bombetta",  "Fusillo",       "Cicciobello",   "Palloncino",   "Joe Biden",  "Trump",
              "Obama",      "De Luca",   "Salvini",       "Renzi",         "Nutella",      "Vespa",      "Mandolino",
              "Ferrari",    "Pavarotti", "Espresso",      "Sir",           "Nocciolina",   "Fluffy",     "Costanzo",
              "Mozart",     "DB",        "Soniuccia",     "Pupi",          "Pupetta",      "Genitore 1", "Genitore 2",
              "Stonks",     "Carotina",  "Waffle",        "Pancake",       "Muffin",       "Cupcake",    "Donut",
              "Jinx",       "Miao",      "Arnold",        "Granita",       "Leone",        "Pangocciolo"};

    std::shuffle(names.begin(), names.end(), randomEngine);
    return names;
}
