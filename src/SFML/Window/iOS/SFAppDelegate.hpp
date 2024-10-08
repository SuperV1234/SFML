#pragma once
#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Window/iOS/WindowImplUIKit.hpp"

#include <UIKit/UIKit.h>

#include <CoreMotion/CoreMotion.h>


////////////////////////////////////////////////////////////
/// \brief Our custom application delegate
///
/// This class handles global application events.
///
////////////////////////////////////////////////////////////
@interface SFAppDelegate : NSObject <UIApplicationDelegate>

////////////////////////////////////////////////////////////
/// \brief Return the instance of the application delegate
///
////////////////////////////////////////////////////////////
+ (SFAppDelegate*)getInstance;

////////////////////////////////////////////////////////////
/// \brief Show or hide the virtual keyboard
///
/// \param visible `true` to show, `false` to hide
///
////////////////////////////////////////////////////////////
- (void)setVirtualKeyboardVisible:(bool)visible;

////////////////////////////////////////////////////////////
/// \brief Get the current touch position for a given finger
///
/// \param index Finger index
///
/// \return Current touch position, or (-1, -1) if no touch
///
////////////////////////////////////////////////////////////
- (sf::Vector2i)getTouchPosition:(unsigned int)index;

////////////////////////////////////////////////////////////
/// \brief Receive an external touch begin notification
///
/// \param index    Finger index
/// \param position Position of the touch
///
////////////////////////////////////////////////////////////
- (void)notifyTouchBegin:(unsigned int)index atPosition:(sf::Vector2i)position;

////////////////////////////////////////////////////////////
/// \brief Receive an external touch move notification
///
/// \param index    Finger index
/// \param position Position of the touch
///
////////////////////////////////////////////////////////////
- (void)notifyTouchMove:(unsigned int)index atPosition:(sf::Vector2i)position;

////////////////////////////////////////////////////////////
/// \brief Receive an external touch end notification
///
/// \param index    Finger index
/// \param position Position of the touch
///
////////////////////////////////////////////////////////////
- (void)notifyTouchEnd:(unsigned int)index atPosition:(sf::Vector2i)position;

////////////////////////////////////////////////////////////
/// \brief Receive an external character notification
///
/// \param character The typed character
///
////////////////////////////////////////////////////////////
- (void)notifyCharacter:(base::U32)character;

////////////////////////////////////////////////////////////
/// \brief Tells if the dimensions of the current window must be flipped when switching to a given orientation
///
/// \param orientation the device has changed to
///
////////////////////////////////////////////////////////////
- (bool)needsToFlipFrameForOrientation:(UIDeviceOrientation)orientation;

////////////////////////////////////////////////////////////
/// \brief Tells if app and view support a requested device orientation or not
///
/// \param orientation the device has changed to
///
////////////////////////////////////////////////////////////
- (bool)supportsOrientation:(UIDeviceOrientation)orientation;

////////////////////////////////////////////////////////////
/// \brief Initializes the factor which is required to convert from points to pixels and back
///
////////////////////////////////////////////////////////////
- (void)initBackingScale;

////////////////////////////////////////////////////////////
// Member data
////////////////////////////////////////////////////////////
@property(nonatomic) sf::priv::WindowImplUIKit* sfWindow;      ///< Main window of the application
@property(readonly, nonatomic) CMMotionManager* motionManager; ///< Instance of the motion manager
@property(nonatomic) CGFloat                    backingScaleFactor;

@end
