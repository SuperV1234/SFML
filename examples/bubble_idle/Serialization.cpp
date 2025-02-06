#include "Serialization.hpp"

#include "Aliases.hpp"
#include "Bubble.hpp"
#include "Cat.hpp"
#include "Countdown.hpp"
#include "Milestones.hpp"
#include "Playthrough.hpp"
#include "Profile.hpp"
#include "PurchasableScalingValue.hpp"
#include "Shrine.hpp"
#include "json.hpp"

#include "SFML/System/Path.hpp"
#include "SFML/System/Vector2.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>


namespace sf
{
////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(modernize-use-constraints)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vector2f, x, y);

} // namespace sf


namespace sf::base
{
////////////////////////////////////////////////////////////
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming, misc-use-internal-linkage)
void to_json(nlohmann::json& j, const Optional<T>& p)
{
    if (p.hasValue())
        j = p.value();
    else
        j = nullptr;
}

////////////////////////////////////////////////////////////
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming, misc-use-internal-linkage)
void from_json(const nlohmann::json& j, Optional<T>& p)
{
    if (j.is_null())
        p.reset();
    else
        p.emplace(j.get<T>());
}

} // namespace sf::base


////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(modernize-use-constraints)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Bubble, position, velocity, scale, rotation, type);

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(modernize-use-constraints)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Cat, type, position, rangeOffset, wobbleRadians, cooldown, hue, inspiredCountdown, nameIdx, hits);

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(modernize-use-constraints)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Shrine, position, wobbleRadians, tcActivation, tcDeath, collectedReward, type);

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(readability-identifier-naming, misc-use-internal-linkage)
void to_json(nlohmann::json& j, const Countdown& p)
{
    j = p.value;
}

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(readability-identifier-naming, misc-use-internal-linkage)
void from_json(const nlohmann::json& j, Countdown& p)
{
    p.value = j;
}

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(readability-identifier-naming, misc-use-internal-linkage)
void to_json(nlohmann::json& j, const TargetedCountdown& p)
{
    j[0] = p.value;
    j[1] = p.startingValue;
}

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(readability-identifier-naming, misc-use-internal-linkage)
void from_json(const nlohmann::json& j, TargetedCountdown& p)
{
    p.value         = j[0];
    p.startingValue = j[1];
}

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(readability-identifier-naming, misc-use-internal-linkage)
void to_json(nlohmann::json& j, const OptionalTargetedCountdown& p)
{
    to_json(j, p.asBase());
}

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(readability-identifier-naming, misc-use-internal-linkage)
void from_json(const nlohmann::json& j, OptionalTargetedCountdown& p)
{
    from_json(j, p.asBase());
}

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(readability-identifier-naming, misc-use-internal-linkage)
void to_json(nlohmann::json& j, const PurchasableScalingValue& p)
{
    j = p.nPurchases;
}

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(readability-identifier-naming, misc-use-internal-linkage)
void from_json(const nlohmann::json& j, PurchasableScalingValue& p)
{
    p.nPurchases = j;
}

////////////////////////////////////////////////////////////
template <SizeT N>
// NOLINTNEXTLINE(readability-identifier-naming, misc-use-internal-linkage)
void to_json(nlohmann::json& j, const PurchasableScalingValue (&p)[N])
{
    j = nlohmann::json::array({p[0].nPurchases, p[1].nPurchases, p[2].nPurchases, p[3].nPurchases, p[4].nPurchases});
}

////////////////////////////////////////////////////////////
template <SizeT N>
// NOLINTNEXTLINE(readability-identifier-naming, misc-use-internal-linkage)
void from_json(const nlohmann::json& j, PurchasableScalingValue (&p)[N])
{
    p[0].nPurchases = j[0];
    p[1].nPurchases = j[1];
    p[2].nPurchases = j[2];
    p[3].nPurchases = j[3];
    p[4].nPurchases = j[4];
}

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(modernize-use-constraints)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    Stats,

    secondsPlayed,
    bubblesPopped,
    bubblesPoppedRevenue,
    bubblesHandPopped,
    bubblesHandPoppedRevenue,
    explosionRevenue,
    flightRevenue);

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(modernize-use-constraints)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    Milestones,

    firstCat,
    firstUnicat,
    firstDevilcat,
    firstAstrocat,

    fiveCats,
    fiveUnicats,
    fiveDevilcats,
    fiveAstrocats,

    tenCats,
    tenUnicats,
    tenDevilcats,
    tenAstrocats,

    prestigeLevel1,
    prestigeLevel2,
    prestigeLevel3,
    prestigeLevel4,
    prestigeLevel5,
    prestigeLevel10,
    prestigeLevel15,
    prestigeLevel20,

    revenue10000,
    revenue100000,
    revenue1000000,
    revenue10000000,
    revenue100000000,
    revenue1000000000);

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(modernize-use-constraints)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    Profile,

    masterVolume,
    musicVolume,
    playAudioInBackground,
    playComboEndSound,
    minimapScale,
    hudScale,
    tipsEnabled,

    statsLifetime);

////////////////////////////////////////////////////////////
// NOLINTNEXTLINE(modernize-use-constraints)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    Playthrough,

    seed,
    nextCatName,

    psvComboStartTime,
    psvMapExtension,
    psvShrineActivation,
    psvBubbleCount,
    psvBubbleValue,
    psvExplosionRadiusMult,

    psvPerCatType,

    psvCooldownMultsPerCatType,

    psvRangeDivsPerCatType,

    psvPPMultiPopRange,
    psvPPInspireDurationMult,
    psvPPManaCooldownMult,
    psvPPManaMaxMult,

    money,

    prestigePoints,

    comboPurchased,
    mapPurchased,

    magicUnlocked,
    manaTimer,
    mana,

    multiPopPurchased,
    smartCatsPurchased,
    geniusCatsPurchased,
    windPurchased,
    astroCatInspirePurchased,

    multiPopEnabled,
    windEnabled,
    geniusCatIgnoreNormalBubbles,
    geniusCatIgnoreStarBubbles,
    geniusCatIgnoreBombBubbles,

    bubbles,
    cats,
    shrines,

    statsTotal,
    statsSession,
    milestones,

    prestigeTipShown,
    shrinesSpawned);

namespace
{
////////////////////////////////////////////////////////////
void forceCopyFile(const std::filesystem::path& from, const std::filesystem::path& to)
{
    std::filesystem::remove(to);
    std::filesystem::copy_file(from, to);
}

////////////////////////////////////////////////////////////
void doRotatingBackup(const std::string& filename)
try
{
    if (sf::Path{filename + ".bak2"}.exists())
        forceCopyFile(filename + ".bak2", filename + ".bak3");

    if (sf::Path{filename + ".bak1"}.exists())
        forceCopyFile(filename + ".bak1", filename + ".bak2");

    if (sf::Path{filename + ".bak0"}.exists())
        forceCopyFile(filename + ".bak0", filename + ".bak1");

    if (sf::Path{filename}.exists())
        forceCopyFile(filename, filename + ".bak0");

} catch (const std::exception& ex)
{
    std::cout << "Failed to backup '" << filename << "' (" << ex.what() << ")\n";
}

} // namespace

////////////////////////////////////////////////////////////
void saveProfileToFile(const Profile& profile)
try
{
    doRotatingBackup("profile.json");
    std::ofstream("profile.json") << nlohmann::json(profile).dump(4);
} catch (const std::exception& ex)
{
    std::cout << "Failed to save profile to file (" << ex.what() << ")\n";
}

////////////////////////////////////////////////////////////
void loadProfileFromFile(Profile& profile)
try
{
    nlohmann::json::parse(std::ifstream{"profile.json"}).get_to(profile);
} catch (const std::exception& ex)
{
    std::cout << "Failed to load profile from file (" << ex.what() << ")\n";
}

////////////////////////////////////////////////////////////
void savePlaythroughToFile(const Playthrough& playthrough)
try
{
    doRotatingBackup("playthrough.json");
    std::ofstream("playthrough.json") << nlohmann::json(playthrough).dump(4);
} catch (const std::exception& ex)
{
    std::cout << "Failed to save playthrough to file (" << ex.what() << ")\n";
}

////////////////////////////////////////////////////////////
void loadPlaythroughFromFile(Playthrough& playthrough)
try
{
    nlohmann::json::parse(std::ifstream{"playthrough.json"}).get_to(playthrough);
} catch (const std::exception& ex)
{
    std::cout << "Failed to load playthrough from file (" << ex.what() << ")\n";
}
