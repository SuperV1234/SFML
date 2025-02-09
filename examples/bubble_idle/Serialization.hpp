#pragma once


////////////////////////////////////////////////////////////
struct Profile;
struct Playthrough;

////////////////////////////////////////////////////////////
void saveProfileToFile(const Profile& profile, const char* filename = "profile.json");
void loadProfileFromFile(Profile& profile, const char* filename = "profile.json");
void savePlaythroughToFile(const Playthrough& playthrough, const char* filename = "playthrough.json");
void loadPlaythroughFromFile(Playthrough& playthrough, const char* filename = "playthrough.json");
