#include "Telegram.h"
#include "Secrets.h"
#include "WiFiComm.h"
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <ArduinoJson.h>

//extern String statusMsg;
String riderName = "";
String helmetID  = "";
String bikeID    = "";


WiFiClientSecure client;
Preferences tgPrefs;

#define MAX_USERS 5
String userIDs[MAX_USERS];

long lastUpdateID = 0;
void handleCommand(const String& text, const String& chat_id, String statusMsg);
void saveUsers();
void loadUsers();
void printUsers();

// ---------------- INIT ----------------
// Initialize Telegram module (no WiFi handling here)
void telegram_init()
{
    client.setInsecure();
    loadUsers();
}

// ---------------- SAVE ----------------
// Save user IDs to non-volatile storage
void saveUsers()
{
    tgPrefs.begin("tg", false);
    for (int i = 0; i < MAX_USERS; i++)
    {
        String id = userIDs[i];
        id.trim();
        if (id == "") id = "--";
        tgPrefs.putString(("u" + String(i)).c_str(), id);
    }
    tgPrefs.end();
}

// ---------------- LOAD ----------------
// Load user IDs from non-volatile storage
void loadUsers()
{
    tgPrefs.begin("tg", true);
    for (int i = 0; i < MAX_USERS; i++)
    {
        String id = tgPrefs.getString(("u" + String(i)).c_str(), "--");
        id.trim();
        if (id == "--") userIDs[i] = "";
        else userIDs[i] = id;
    }
    tgPrefs.end();
    Serial.println("Users ID loaded:");
    printUsers();
}

// ---------------- ENCODE ----------------
// URL encode message for HTTP transmission
String encode(String msg)
{
    String encoded = "";

    for (int i = 0; i < msg.length(); i++)
    {
        char c = msg[i];

        //  allow HTML characters
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' ||
            c == '<' || c == '>' || c == '/' || c == '=' || c == '"' )
        {
            encoded += c;
        }
        else if (c == ' ')
        {
            encoded += "%20";
        }
        else if (c == '\n')
        {
            encoded += "%0A";
        }
        else
        {
            char buf[4];
            sprintf(buf, "%%%02X", c);
            encoded += buf;
        }
    }
    return encoded;
}

// ---------------- SEND ----------------
// Send message to a specific Telegram user
void telegram_sendTo(const String& chat_id, const String& message)
{
    if (!WiFiComm_isConnected()) return;
    WiFiClientSecure client;
    client.setInsecure();
    if (!client.connect("api.telegram.org", 443))
    {
        Serial.println("Connection failed");
        return;
    }
    String url = String("/bot") + BOT_TOKEN +
                 "/sendMessage?chat_id=" + chat_id +
                 "&text=" + encode(message) ;// +
                 //"&parse_mode=HTML";   //  IMPORTANT;
    client.print(
        String("GET ") + url + " HTTP/1.1\r\n" +
        "Host: api.telegram.org\r\n" +
        "Connection: close\r\n\r\n"
    );
    unsigned long start = millis();
    while (!client.available() && millis() - start < 200) yield();
    while (client.available()) client.read();
    client.stop();
    Serial.println(message);
}

// ---------------- FETCH ----------------
// Fetch updates from Telegram server
bool fetchUpdates(String& response)
{
    if (!WiFiComm_isConnected()) return false;
    if (!client.connected())
    {
        if (!client.connect("api.telegram.org", 443))
            return false;
    }
    String url = String("/bot") + BOT_TOKEN +
                 "/getUpdates?offset=" + String(lastUpdateID + 1);
    client.print(
        "GET " + url + " HTTP/1.1\r\n"
        "Host: api.telegram.org\r\n"
        "Connection: close\r\n\r\n"
    );
    unsigned long start = millis();
    while (!client.available())
    {
        if (millis() - start > 120) return false;
        yield();
    }
    response = "";
    unsigned long lastDataTime = millis();
    while (client.available() || client.connected())
    {
        while (client.available())
        {
            char c = client.read();
            if (response.length() < 4000) response += c;
            lastDataTime = millis();
        }
        if (millis() - lastDataTime > 120) break;
        yield();
    }
    return true;
}

// ---------------- EXTRACT ----------------
// Extract message, chat ID and update ID from response
bool extractMessage(const String& res, String& text, String& chat_id, long& update_id)
{
    int uid = res.indexOf("\"update_id\":");
    if (uid == -1) return false;
    update_id = res.substring(uid + 12, res.indexOf(",", uid)).toInt();
    int cid = res.indexOf("\"chat\"");
    if (cid == -1) return false;
    int idPos = res.indexOf("\"id\":", cid);
    if (idPos == -1) return false;
    int start = idPos + 5, end = start;
    while (end < res.length() && isDigit(res[end])) end++;
    chat_id = res.substring(start, end);
    int txt = res.indexOf("\"text\":\"");
    if (txt == -1) return false;
    int s = txt + 8, e = s;
    while (e < res.length())
    {
        if (res[e] == '"' && res[e - 1] != '\\') break;
        e++;
    }
    text = res.substring(s, e);
    text.trim();
    return true;
}

// ---------------- LOOP ----------------
// Process incoming Telegram updates
void telegram_loop(String statusMsg)
{
    if (!WiFiComm_isConnected()) return;
    String response;
    if (!fetchUpdates(response)) return;
    if (response.indexOf("\"result\":[{") == -1) return;
    String text, chat_id;
    long update_id;
    if (!extractMessage(response, text, chat_id, update_id)) return;
    lastUpdateID = update_id;
    handleCommand(text, chat_id, statusMsg);
}

// ---------------- BROADCAST ----------------
// Send message to all registered users
void telegram_sendMessage(const String& message)
{
    Serial.println("Sending message to all users...");
   // message = "<pre>\n" + message + "\n</pre>"; //NEW
    bool sent = false;
    for (int i = 0; i < MAX_USERS; i++)
    {
        String id = userIDs[i];
        id.trim();
        if (id != "")
        {
            int sp = id.lastIndexOf(' ');
            String chatID = (sp != -1) ? id.substring(sp + 1) : id;
            telegram_sendTo(chatID, message);
            sent = true;
        }
    }
    if (!sent)
    {
        Serial.println("No users found. Sending to admin.");
        telegram_sendTo(ADMIN_CHAT_ID, message);
    }
}

// ---------------- STATUS ----------------
// Check WiFi status via WiFiComm module
bool telegram_isWiFiConnected()
{
    return WiFiComm_isConnected();
}

// ---------------- PRINT ----------------
void printUsers()
{
    Serial.println("---- Emergency Contacts ----");
    for (int i = 0; i < MAX_USERS; i++)
    {
        Serial.print("Emergency Contact - ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(userIDs[i].length() ? userIDs[i] : "--");
    }
}

String cleanChatID(String id)
{
    id.trim();
    id.replace("\n", "");
    id.replace("\r", "");

    String clean = "";
    for (int i = 0; i < id.length(); i++)
    {
        if (isDigit(id[i]))
            clean += id[i];
    }
    return clean;
}

String buildUserList(const String& header)
{
    String msg = header;
    for (int i = 0; i < MAX_USERS; i++)
    {
        msg += "Emergency Contact - " + String(i + 1) + ": " +
               (userIDs[i].length() ? userIDs[i] : "--");

        if (i < MAX_USERS - 1) msg += "\n";
    }
    return msg;
}

void notifyUser(const String& chatID, const String& name, bool added)
{
    String msg;

    if (added)
    {
        msg ="You have been added as an Emergency Contact\n\n"
             "Name: " + name + "\n"
             "You will receive alerts from this system.";
    }
    else
    {
        msg = "❌ You have been removed as an Emergency Contact\n\n"
              "Name: " + name + "\n"
              "You will no longer receive alerts from this system.";
    }
    telegram_sendTo(chatID, msg);
}

bool isRegisteredUser(const String& chat_id)
{
    String incomingID = cleanChatID(chat_id);

    for (int i = 0; i < MAX_USERS; i++)
    {
        if (userIDs[i].length())
        {
            int sp = userIDs[i].lastIndexOf(' ');
            if (sp == -1)
                continue;

            String storedID = cleanChatID(userIDs[i].substring(sp + 1));

            if (storedID == incomingID)
            {
                Serial.println("MATCH FOUND ✅");
                return true;
            }
        }
    }

    Serial.println("NO MATCH ❌");
    return false;
}

// ---------------- COMMANDS ----------------
void handleCommand(const String& text, const String& chat_id, String statusMsg)
{
    Serial.println("MSG: " + text + " | CHAT ID: " + chat_id);
      String cmd, arg;
    int sp = text.indexOf(' ');
    if (sp == -1) cmd = text;
    else { cmd = text.substring(0, sp); arg = text.substring(sp + 1); arg.trim(); }
    bool isUser = isRegisteredUser(chat_id);
    String adminHelp =
        "🛠 Admin Commands:\n\n"
        "➕ Add emergency contact\n"
        "/adduser [name] [chat_id]\n\n"
        "❌ Remove emergency contact\n"
        "/deleteuser [name] [chat_id]\n\n"
        "📋 View emergency contacts\n"
        "/listusers\n\n"
        "⚠ Delete all emergency contacts\n"
        "/deleteall\n\n"
        "⚙ Set rider configuration\n"
        "/setconfig [name] [helmetID] [bikeID]\n\n"
        "📄 View configuration\n"
        "/readconfig\n\n"
        "🗑 Delete configuration\n"
        "/deleteconfig\n\n"
        "📡 System status\n"
        "/status\n\n"
        "ℹ Show this menu\n"
        "/help";

        String userHelp =
        "📋 User Commands:\n\n"
        "📡 Check system status\n"
        "/status\n\n"
        "📋 View emergency contacts\n"
        "/listusers\n\n"
        "ℹ Show this menu\n"
        "/help";

    // STATUS
    if (cmd == "/status")
    {
        if (chat_id == ADMIN_CHAT_ID || isUser)
            telegram_sendTo(chat_id, "Rider Status \n" + statusMsg);
        else
            telegram_sendTo(chat_id, "Access denied");
        return;
    }

    // LIST USERS
    if (cmd == "/listusers")
    {
        if (chat_id == ADMIN_CHAT_ID || isUser)
        {
            telegram_sendTo(chat_id,
                buildUserList("Emergency Contacts\n"));
        }
        else
            telegram_sendTo(chat_id, "Access denied");
        return;
    }

    // HELP
    if (cmd == "/help")
    {
        if (chat_id == ADMIN_CHAT_ID)
            telegram_sendTo(chat_id, adminHelp);
        else if (isUser)
            telegram_sendTo(chat_id, userHelp);
        else
            telegram_sendTo(chat_id, "Access denied");
        return;
    }

    // NON ADMIN
    if (chat_id != ADMIN_CHAT_ID)
    {
        if (isUser)
            telegram_sendTo(chat_id, "Unknown command");
        else
            telegram_sendTo(chat_id, "Access denied");
        return;
    }

    // ADD USER
    if (cmd == "/adduser")
    {
        int sp2 = arg.indexOf(' ');
        if (sp2 == -1)
        {
            telegram_sendTo(chat_id, "Usage: /adduser <name> <chat_id>");
            return;
        }
        String name   = arg.substring(0, sp2);
        String chatID = cleanChatID(arg.substring(sp2 + 1));
        String entry  = name + " " + chatID;
        for (int i = 0; i < MAX_USERS; i++)
        {
            if (userIDs[i].endsWith(chatID))
            {
                telegram_sendTo(chat_id, "User already exists");
                return;
            }
        }
        for (int i = 0; i < MAX_USERS; i++)
        {
            if (userIDs[i] == "")
            {
                userIDs[i] = entry;
                saveUsers();
                telegram_sendTo(ADMIN_CHAT_ID,
                    buildUserList("✅ User Added:\n" + entry + "\n\n📋 Updated User List:\n"));
                notifyUser(chatID, name, true);
                return;
            }
        }
        telegram_sendTo(chat_id, "Limit reached");
    }

    // DELETE USER
    else if (cmd == "/deleteuser")
    {
        int sp2 = arg.indexOf(' ');
        if (sp2 == -1)
        {
            telegram_sendTo(chat_id, "Usage: /deleteuser <name> <chat_id>");
            return;
        }
        String name   = arg.substring(0, sp2);
        String chatID = cleanChatID(arg.substring(sp2 + 1));
        bool found = false;
        String deletedEntry = "";
        for (int i = 0; i < MAX_USERS; i++)
        {
            if (userIDs[i] == "") continue;
            String entry = userIDs[i];
            int sp = entry.lastIndexOf(' ');
            String storedID   = entry.substring(sp + 1);
            String storedName = entry.substring(0, sp);
            if (storedID == chatID && storedName.equalsIgnoreCase(name))
            {
                deletedEntry = entry;
                userIDs[i] = "";
                found = true;
                break;
            }
        }
        if (!found)
        {
            telegram_sendTo(chat_id, "Emergency Contact not found");
            return;
        }
        saveUsers();
        notifyUser(chatID, name, false);
        telegram_sendTo(ADMIN_CHAT_ID,
            buildUserList("❌ Emergency Contact Deleted:\n" + deletedEntry + "\n\n📋 Updated Emergency Contact List:\n"));
    }

    // DELETE ALL
    else if (cmd == "/deleteall")
    {
        for (int i = 0; i < MAX_USERS; i++)
        {
            if (userIDs[i].length())
            {
                String entry = userIDs[i];
                int sp = entry.lastIndexOf(' ');
                String name   = entry.substring(0, sp);
                String chatID = cleanChatID(entry.substring(sp + 1));
                notifyUser(chatID, name, false);
            }
        }
        for (int i = 0; i < MAX_USERS; i++)
            userIDs[i] = "";
        saveUsers();
        telegram_sendTo(ADMIN_CHAT_ID,
            buildUserList("❌ All Emergency Contacts Deleted\n\n📋 Updated Emergency Contact List:\n"));
    }
    // -------- SET CONFIG --------
    else if (cmd == "/setconfig")
    {
        int p1 = arg.indexOf(' ');
        int p2 = arg.indexOf(' ', p1 + 1);

        if (p1 == -1 || p2 == -1)
        {
            telegram_sendTo(chat_id,
                "Usage: /setconfig <name> <helmetID> <bikeID>");
            return;
        }

        String name   = arg.substring(0, p1);
        String helmet = arg.substring(p1 + 1, p2);
        String bike   = arg.substring(p2 + 1);

        Preferences prefs;
        prefs.begin("config", false);

        prefs.putString("rider", name);
        prefs.putString("helmet", helmet);
        prefs.putString("bike", bike);

        prefs.end();

        riderName = name;
        helmetID  = helmet;
        bikeID    = bike;

        String msg =
            "✅ Configuration Saved\n\n"
            "👤 Rider: " + riderName + "\n"
            "🪖 Helmet: " + helmetID + "\n"
            "🏍 Bike: " + bikeID;

        telegram_sendTo(chat_id, msg);
    }
    // -------- READ CONFIG (ADMIN ONLY) --------
    else if (cmd == "/readconfig")
    {
        // load latest from storage (optional but safe)
        Preferences prefs;
        prefs.begin("config", true);
        String name   = prefs.getString("rider", "UNKNOWN");
        String helmet = prefs.getString("helmet", "H-000");
        String bike   = prefs.getString("bike", "B-000");
        prefs.end();
        String msg =
            "📋 Current Configuration\n\n"
            "👤 Rider: " + name + "\n"
            "🪖 Helmet: " + helmet + "\n"
            "🏍 Bike: " + bike;
        telegram_sendTo(chat_id, msg);
    }
    // -------- DELETE CONFIG (ADMIN ONLY) --------
    else if (cmd == "/deleteconfig")
    {
        Preferences prefs;
        prefs.begin("config", false);
        // remove keys
        prefs.remove("rider");
        prefs.remove("helmet");
        prefs.remove("bike");
        prefs.end();
        // reset runtime variables
        riderName = "";
        helmetID  = "";
        bikeID    = "";
        String msg =
            "❌ Configuration Deleted\n\n"
            "All stored configuration has been cleared.";
        telegram_sendTo(chat_id, msg);
    }
    else
    {
        telegram_sendTo(chat_id, "Unknown command");
    }
}