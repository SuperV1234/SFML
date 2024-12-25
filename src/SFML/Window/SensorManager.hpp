#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Window/Sensor.hpp"
#include "SFML/Window/SensorImpl.hpp"

#include "SFML/Base/EnumArray.hpp"


namespace sf::priv
{
////////////////////////////////////////////////////////////
/// \brief Global sensor manager
///
////////////////////////////////////////////////////////////
class [[nodiscard]] SensorManager
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit SensorManager();

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~SensorManager();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    SensorManager(const SensorManager&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    SensorManager& operator=(const SensorManager&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Check if a sensor is available on the underlying platform
    ///
    /// \param sensor Sensor to check
    ///
    /// \return `true` if the sensor is available, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    bool isAvailable(Sensor::Type sensor);

    ////////////////////////////////////////////////////////////
    /// \brief Enable or disable a sensor
    ///
    /// \param sensor  Sensor to modify
    /// \param enabled Whether it should be enabled or not
    ///
    ////////////////////////////////////////////////////////////
    void setEnabled(Sensor::Type sensor, bool enabled);

    ////////////////////////////////////////////////////////////
    /// \brief Check if a sensor is enabled
    ///
    /// \param sensor Sensor to check
    ///
    /// \return `true` if the sensor is enabled, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isEnabled(Sensor::Type sensor) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the current value of a sensor
    ///
    /// \param sensor Sensor to read
    ///
    /// \return Current value of the sensor
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vector3f getValue(Sensor::Type sensor) const;

    ////////////////////////////////////////////////////////////
    /// \brief Update the state of all the sensors
    ///
    ////////////////////////////////////////////////////////////
    void update();

private:
    ////////////////////////////////////////////////////////////
    /// \brief Sensor information and state
    ///
    ////////////////////////////////////////////////////////////
    struct Item
    {
        bool       available{}; //!< Is the sensor available on this device?
        bool       enabled{};   //!< Current enable state of the sensor
        SensorImpl sensor{};    //!< Sensor implementation
        Vector3f   value;       //!< The current sensor value
    };

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    base::EnumArray<Sensor::Type, Item, Sensor::Count> m_sensors; //!< Sensors information and state
};

} // namespace sf::priv
