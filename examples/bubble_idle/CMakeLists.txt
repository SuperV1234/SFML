# all source files
set(SRC BubbleIdle.cpp Serialization.cpp)

# define the bubble_idle target
sfml_add_example(bubble_idle GUI_APP
                 SOURCES ${SRC}
                 DEPENDS SFML::Audio SFML::Graphics SFML::ImGui ImGui::ImGui)
