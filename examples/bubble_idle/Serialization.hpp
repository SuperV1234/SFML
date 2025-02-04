#pragma once

struct Profile;
struct Playthrough;

void saveProfileToFile(const Profile& profile);
void loadProfileFromFile(Profile& profile);
void savePlaythroughToFile(const Playthrough& playthrough);
void loadPlaythroughFromFile(Playthrough& playthrough);
