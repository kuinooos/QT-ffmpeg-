file(REMOVE_RECURSE
  "PlayerGui/qml/Main.qml"
  "PlayerGui/qml/components/ControlButton.qml"
  "PlayerGui/qml/components/GlassCard.qml"
  "PlayerGui/qml/components/TimelineSlider.qml"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/app_ffmpeg_player_gui_tooling.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
