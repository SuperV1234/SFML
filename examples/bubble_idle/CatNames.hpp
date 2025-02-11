#pragma once

#include "Cat.hpp"
#include "ExactArray.hpp"

#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/Span.hpp"

#include <algorithm>
#include <string>
#include <vector>


// IDEAS: Shodan, Glados,

////////////////////////////////////////////////////////////
inline constexpr const char* normalCatNames[] =
    {"Mochi",      "Biscuit", "Tuna",      "Waffle",    "Noodle",     "Pebble",    "Pickle",      "Pudding", "Sprout",
     "Boop",       "Nacho",   "Dandelion", "Sushi",     "Snickers",   "Muffin",    "Zippy",       "Pumpkin", "Gizmo",
     "Wobble",     "Cupcake", "Mittens",   "Doodle",    "Beans",      "Rawr",      "Marshmallow", "Fizz",    "Whiskers",
     "Pounce",     "Olive",   "Honey",     "Pepper",    "Nibble",     "Cinnamon",  "Jinx",        "Toffee",  "Zorro",
     "Squish",     "Pogo",    "Purrseus",  "Shadow",    "Pixel",      "Bubbly",    "Yoshi",       "Dizzy",   "Loki",
     "Taco",       "Ziggy",   "Snuggles",  "Cuddles",   "Gorgonzola", "Pistachio", "Mozzarella",  "Nutella", "Cookie",
     "Strawberry", "Cloud",   "Espresso",  "Peanut",    "Donut",      "Vader",     "Yoda",        "Gandalf", "Bilbo",
     "Leia",       "Sonic",   "Mario",     "Luigi",     "Zelda",      "Link",      "Neo",         "Tron",    "Stark",
     "Draco",      "Hodor",   "Khaleesi",  "Dovahkiin", "Nuka",       "Cortana",   "Tardis",      "Rogue",   "Frodo",
     "Gimli",      "Legolas", "Bender",    "Zim",       "Stan",       "Cartman",   "Kenny",       "Kyle",    "Butters",
     "Randy",      "Ike",     "Chef",      "Timmy",     "Homer",      "Bart",      "Marge",       "Lisa",    "Maggie",
     "Flanders",   "Reimu",   "Marisa",    "Cirno",     "Remilia",    "Sakuya",    "Youmu",       "Yuyuko",  "Reisen",
     "Tewi",       "Aya",     "Sanae",     "Kanako",    "Suwako",     "Beethro",   "Halph",       "Dugan",   "Denfry",
     "Tendry",     "Gunthro", "Cannolo",   "Braciola",  "Granita"};

////////////////////////////////////////////////////////////
inline constexpr const char* uniCatNames[] =
    {"Orion",   "Cassiopeia",  "Andromeda", "Ursa",      "Scorpius", "Lyra",    "Cygnus",    "Pegasus",
     "Hydra",   "Capricornus", "Aquarius",  "Centaurus", "Corvus",   "Crux",    "Delphinus", "Eridanus",
     "Lepus",   "Monoceros",   "Ophiuchus", "Eris",      "Cassie",   "Capella", "Auriga",    "Pavo",
     "Atria",   "Aquila",      "Aries",     "Gemini",    "Leo",      "Libra",   "Pisces",    "Sagittarius",
     "Scorpio", "Taurus",      "Virgo",     "Polaris",   "Lucero",   "Halley",  "Comet",     "Astrid",
     "Satoru",  "Shun",        "Saki",      "Mamoru",    "Maria"};

////////////////////////////////////////////////////////////
inline constexpr const char* devilCatNames[] = {
    "Atronach",       "Inferno",     "Satan",      "Lucifer",   "Beelzebub", "Mephisto",   "Diablo",       "Baphomet",
    "Belial",         "Moloch",      "Asmodeus",   "Leviathan", "Behemoth",  "Krampus",    "Azazel",       "Abaddon",
    "Astaroth",       "Lilith",      "Mammon",     "Baal",      "Samael",    "Belphegor",  "Nyarlathotep", "Cthulhu",
    "Shub-Niggurath", "Yog-Sothoth", "Hastur",     "Dagon",     "Ithaqua",   "Tsathoggua", "Nodens",       "Yig",
    "Ghatanothoa",    "Yibb-Tstll",  "Zoth-Ommog", "Cthugha",   "Hypnos",    "Nug",        "Yogash",       "Zoth",
};

////////////////////////////////////////////////////////////
inline constexpr const char* astroCatNames[] =
    {"Armstrong", "Buzz",  "Apollo",   "Lightyear", "Aldrin",   "Gagarin",       "Sputnik", "Vostok",
     "Fry",       "Leela", "Zoidberg", "Shenzhou",  "Tiangong", "Spock",         "Picard",  "Cooper",
     "Groot",     "Kirk",  "Spock",    "Hadfield",  "Jamison",  "Cristoforetti", "Solo",    "Shepard"};


////////////////////////////////////////////////////////////
inline constexpr const char* wizardCatNames[] = {
    "Gandalf",
    "Dumbledore",
    "Harry",
    "Merlin",
    "Xerath",
    "Zilean",
    "Veigar",
    "Ryze",
};

////////////////////////////////////////////////////////////
inline constexpr const char* mouseCatNames[] =
    {"Clicker",
     "Squeaky",
     "Nibbler",
     "Twitch",
     "Squealer",
     "Kiroumaru",
     "Yakomaru",
     "Splinter",
     "Remy",
     "Jerry",
     "Mickey",
     "Fievel"};

////////////////////////////////////////////////////////////
inline constexpr const char* engiCatNames[] = {
    "Stark",
    "Tesla",
    "Edison",
    "Watt",
    "Volta",
    "Faraday",
    "Freeman",
    "Torbjorn",
    "Bulma",
    "Kaylee",
    "Scotty",
    "Brown",
    "Sanchez",
    "Ratchet",
    "Wily",
};

////////////////////////////////////////////////////////////
inline constexpr const char* repulsoCatNames[] = {
    "Janna",
    "Zephyr",
    "Storm",
    "Aang",
    "Korra",
    "Venti",
    "Sciangazza",
    "Aeolus",
};

////////////////////////////////////////////////////////////
inline constexpr const char* attractoCatNames[] = {
    "Magneto",
    "Polaris",
    "Magnemite",
    "Neodymium",
    "Ferrite",
};

////////////////////////////////////////////////////////////
inline constexpr EXACT_ARRAY(
    sf::base::Span<const char* const>,
    catNamesPerType,
    nCatTypes,
    {
        {normalCatNames, sf::base::getArraySize(normalCatNames)}, // Normal
        {uniCatNames, sf::base::getArraySize(uniCatNames)},       // Uni
        {devilCatNames, sf::base::getArraySize(devilCatNames)},   // Devil
        {normalCatNames, sf::base::getArraySize(normalCatNames)}, // Witch (TODO)
        {astroCatNames, sf::base::getArraySize(astroCatNames)},   // Astro

        {wizardCatNames, sf::base::getArraySize(wizardCatNames)},   // Wizard
        {mouseCatNames, sf::base::getArraySize(mouseCatNames)},     // Mouse
        {engiCatNames, sf::base::getArraySize(engiCatNames)},       // Engi
        {repulsoCatNames, sf::base::getArraySize(repulsoCatNames)}, // Repulso
    });

////////////////////////////////////////////////////////////
[[nodiscard]] std::vector<std::string> getShuffledCatNames(const CatType catType, auto&& randomEngine)
{
    const auto span = catNamesPerType[asIdx(catType)];

    std::vector<std::string> names{span.begin(), span.end()};
    std::shuffle(names.begin(), names.end(), randomEngine);

    return names;
}
