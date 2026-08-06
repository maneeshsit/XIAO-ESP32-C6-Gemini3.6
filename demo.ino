#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ======================
// WiFi Credentials
// ======================
const char* ssid = "";
const char* password = "";

// ======================
// Gemini API Key
// ======================
const char* Gemini_Token = "YOUR_TOKEN";

// Current Gemini endpoint
const char* Gemini_URL =
"https://generativelanguage.googleapis.com/v1beta/models/gemini-3.6-flash:generateContent";

// HTTPS Client
WiFiClientSecure secureClient;
HTTPClient https;

// User prompt
String prompt;

//------------------------------------------------------------

bool connectWiFi()
{
    Serial.print("Connecting to WiFi");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);

        if (millis() - start > 30000)
        {
            Serial.println("\nWiFi Timeout");
            return false;
        }
    }

    Serial.println();
    Serial.println("WiFi Connected");

    Serial.print("IP Address : ");
    Serial.println(WiFi.localIP());

    Serial.print("RSSI : ");
    Serial.println(WiFi.RSSI());

    return true;
}

//------------------------------------------------------------

bool sendToGemini(String question)
{
    secureClient.setInsecure();

    https.setConnectTimeout(30000);
    https.setTimeout(30000);

    if (!https.begin(secureClient, Gemini_URL))
    {
        Serial.println("https.begin() FAILED");
        return false;
    }

    https.addHeader("Content-Type", "application/json");
    https.addHeader("x-goog-api-key", Gemini_Token);

    JsonDocument doc;

    JsonArray contents = doc["contents"].to<JsonArray>();

    JsonObject content = contents.add<JsonObject>();

    JsonArray parts = content["parts"].to<JsonArray>();

    JsonObject part = parts.add<JsonObject>();

    part["text"] = question;

    JsonObject generationConfig =
        doc["generationConfig"].to<JsonObject>();

    generationConfig["temperature"] = 0.7;
    generationConfig["topP"] = 0.95;
    generationConfig["topK"] = 40;
    generationConfig["maxOutputTokens"] = 1024;

    String payload;

    serializeJson(doc, payload);

    Serial.println();
    Serial.println("==============================");
    Serial.println("Request Payload");
    Serial.println("==============================");
    Serial.println(payload);

    Serial.println();
    Serial.println("Sending request...");

    int httpCode = https.POST(payload);

    Serial.print("HTTP Code : ");
    Serial.println(httpCode);

    String response = https.getString();

    Serial.println();
    Serial.println("==============================");
    Serial.println("Raw Response");
    Serial.println("==============================");
    Serial.println(response);

    if (httpCode != HTTP_CODE_OK)
    {
        https.end();
        return false;
    }

    // -------- Response parsing continues in Part 2 --------

        // Parse JSON response
    JsonDocument responseDoc;

    DeserializationError err = deserializeJson(responseDoc, response);

    if (err)
    {
        Serial.print("JSON Parse Error: ");
        Serial.println(err.c_str());

        https.end();
        return false;
    }

    // Check for API error
    if (responseDoc["error"].is<JsonObject>())
    {
        Serial.println("\nGemini API Error:");

        if (responseDoc["error"]["code"])
        {
            Serial.print("Code : ");
            Serial.println((int)responseDoc["error"]["code"]);
        }

        if (responseDoc["error"]["message"])
        {
            Serial.print("Message : ");
            Serial.println((const char*)responseDoc["error"]["message"]);
        }

        https.end();
        return false;
    }

    String answer = "";

    if (responseDoc["candidates"][0]["content"]["parts"][0]["text"])
    {
        answer =
            responseDoc["candidates"][0]["content"]["parts"][0]["text"]
                .as<String>();
    }

    Serial.println();
    Serial.println("==============================");
    Serial.println("Gemini Response");
    Serial.println("==============================");
    Serial.println(answer);

    https.end();

    return true;
}

//------------------------------------------------------------

void setup()
{
    Serial.begin(115200);
    delay(3000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("XIAO ESP32-C6 Gemini Chat");
    Serial.println("========================================");

    if (!connectWiFi())
    {
        Serial.println("Cannot continue.");
        while (true)
            delay(1000);
    }

    Serial.println();
    Serial.println("Ready.");
    Serial.println("Type a prompt and press ENTER.");
}

//------------------------------------------------------------

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi Lost. Reconnecting...");
        connectWiFi();
    }

    if (Serial.available())
    {
        prompt = Serial.readStringUntil('\n');
        prompt.trim();

        if (prompt.length() == 0)
            return;

        Serial.println();
        Serial.print("You: ");
        Serial.println(prompt);

        bool ok = sendToGemini(prompt);

        if (!ok)
        {
            Serial.println();
            Serial.println("Request failed.");
        }

        Serial.println();
        Serial.println("------------------------------------");
        Serial.println("Ask another question...");
    }
}