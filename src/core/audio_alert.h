#ifndef AUDIO_ALERT_H
#define AUDIO_ALERT_H

#include <string>

namespace AudioAlert {

const std::string& reminderSoundPath();
bool canPlayReminderSound(std::string& message);
bool playReminderSoundBlocking(std::string& message);
void playReminderSoundAsync();

}

#endif
