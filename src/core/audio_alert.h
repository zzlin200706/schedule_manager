#ifndef AUDIO_ALERT_H
#define AUDIO_ALERT_H

/*
 * 文件职责：
 * 声明提醒音频播放接口。CLI 的 Reminder 和 Qt GUI 的 MainWindow
 * 共用这一实现来查找并播放 resources/alarm.mp3。
 */

#include <string>

namespace AudioAlert {

const std::string& reminderSoundPath();
bool canPlayReminderSound(std::string& message);
bool playReminderSoundBlocking(std::string& message);
void playReminderSoundAsync();

}

#endif
