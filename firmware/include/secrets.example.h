#ifndef SECRETS_EXAMPLE_H
#define SECRETS_EXAMPLE_H

// =============================================================================
// CREDENTIALS CONFIGURATION TEMPLATE
// Copy this file to "secrets.h" and enter your actual credentials.
// "secrets.h" is ignored by .gitignore to prevent accidental credential leakage.
// =============================================================================

// Wi-Fi Access Point Credentials
#define WIFI_SSID           "YOUR_WIFI_SSID"
#define WIFI_PASSWORD       "YOUR_WIFI_PASSWORD"

// Telegram Bot Configuration
// Obtain via BotFather on Telegram
#define TELEGRAM_BOT_TOKEN  "1234567890:ABCdefGhIJKlmNoPQRsTUVwxyZ"

// Target Chat ID (User ID or Group Chat ID)
// Can be retrieved using @userinfobot or https://api.telegram.org/bot<TOKEN>/getUpdates
#define TELEGRAM_CHAT_ID    "987654321"

#endif // SECRETS_EXAMPLE_H

