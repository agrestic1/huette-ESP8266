#include "deviceData.hpp"

const char *properties[4] = {"name", "type", "on_state", "brightness"};

DeviceData::DeviceData()
{
    this->resetType();

    if (this->data.type != "Light")
    {
        strncpy(this->data.type, DEFAULT_DEVICE_TYPE, MAX_STRING_SIZE);
        strncpy(this->data.name, DEFAULT_DEVICE_NAME, MAX_STRING_SIZE);
        this->data.on_state = DEFAULT_DEVICE_ON_STATE;
        this->data.brightness = DEFAULT_DEVICE_BRIGHTNESS;
    }
    else
    {
        this->resetName();
        this->resetOnState();
        this->resetBrightness();
    }

    updateOutput();
}

int DeviceData::Deserialize(JsonDocument &doc, const char *jsonString)
{
    // Deserialize the JSON string (remember doc is only MAX_JSON_SIZE bytes wide, so pass 'inputSize' also)
    DeserializationError error = deserializeJson(doc, jsonString, (size_t)MAX_JSON_SIZE);
    // Test if parsing succeeds.
    if (error)
    {
        // Serial.print(F("deserializeJson() failed: "));
        // Serial.println(error.c_str());
        DEBUG_PRINT("deserializeJson() failed: ");
        DEBUG_PRINTLN(error.c_str());

        // Todo: Send error message over to backend using Socket.emit
        // Better: Define other error levels (-2, -3 ..) To indicate type of error (using enum)
        return -1;
    }
    else
    {
        // Success
        return 0;
    }
}

int DeviceData::Parse(JsonDocument &doc, CommandOptions command)
{

    bool changed[PROPERTY_COUNT];
    int changedIt = 0;

    if (command > COMMAND_PUBLISH)
    {
        // Always implies set of internal properties if > COMMAND_PUBLISH
        // keep track if a property was actually given or not
        // This is simply to avoid parsing the properties all over again for the next step...
        // Using postincrement means: first read value, then increment
        changed[changedIt++] = this->setName(doc["name"].as<JsonVariant>());
        changed[changedIt++] = this->setType(doc["type"].as<JsonVariant>());
        changed[changedIt++] = this->setOnState(doc["on_state"].as<JsonVariant>());
        changed[changedIt] = this->setBrightness(doc["brightness"].as<JsonVariant>());
    }

    if (command == COMMAND_WRITE_EEPROM)
    {
        // is higher than COMMAND_GET so they are already locally available
        changedIt = 0;
        // Do that incrementing stuff again
        if (changed[changedIt++])
            this->storeName();
        if (changed[changedIt++])
            this->storeType();
        if (changed[changedIt++])
            this->storeOnState();
        if (changed[changedIt])
            this->storeBrightness();
    }

    // Todo: Check validity

    // COMMAND_GET is always implied, we send back the actual state of our device
    // Note the reverse order of 'changedIt'
    if (changed[changedIt--])
        doc["brightness"].set(this->data.brightness);
    if (changed[changedIt--])
        doc["on_state"].set(this->data.on_state);
    if (changed[changedIt--])
        doc["type"].set(this->data.type);
    if (changed[changedIt])
        doc["name"].set(this->data.name);

    return 0;
}

int DeviceData::Serialize(const JsonDocument &doc, char *jsonString)
{
    // Computes the length of the minified JSON document that serializeJson() produces, excluding the null-terminator.
    // But we also want null termination so: + 1
    size_t len = measureJson(doc) + 1;
    // Reallocate memory for our JSON string buffer
    jsonString = (char *)realloc(jsonString, len * sizeof(char));
    // Serialize the JSON document
    size_t count = serializeJson(doc, jsonString, len);
    // Test if parsing succeeds.
    if (count == 0)
    {
        // Serial.print(F("serializeJson() failed: "));
        // Serial.println("No bytes written");
        DEBUG_PRINT("serializeJson() failed: ");
        DEBUG_PRINTLN("No bytes written");

        // Todo: Send error message over to backend using Socket.emit
        // Better: Define other error levels (-2, -3 ..) To indicate type of error (using enum)
        return -1;
    }
    else
    {
        // Success
        return 0;
    }
}

void DeviceData::debugPrint(void)
{
    DEBUG_PRINTLN(this->data.name);
    DEBUG_PRINTLN(this->data.type);
    DEBUG_PRINTLN(this->data.on_state ? "True" : "False");
    DEBUG_PRINTLN(this->data.brightness);
}

void DeviceData::updateOutput(void)
{
#ifdef DEFLED_BUILTIN
    if (this->data.on_state)
    {
        analogWrite(LED_BUILTIN, PWM_RANGE - this->data.brightness);
    }
    else
    {
        analogWrite(LED_BUILTIN, PWM_RANGE);
    }
#else
    if (this->data.on_state)
    {
        analogWrite(PWM_PIN, this->data.brightness);
    }
    else
    {
        analogWrite(PWM_PIN, 0);
    }
#endif
    DEBUG_PRINT("Brightness changed to ");
    DEBUG_PRINTLN(this->data.brightness);
}

bool DeviceData::setName(JsonVariant value)
{
    // See API note regarding '.containsKey()': https://arduinojson.org/v6/api/jsonobject/containskey/
    if (!value.isNull())
    {
        const char *name = value.as<const char *>();
        // The C library function size_t strlen(const char *str) computes the length of the string str up to, but not including the terminating null character.
        // But we also want null termination so: + 1
        // size_t length = strlen(name) + 1;
        // Reallocate memory for private member 'name'
        // this->data.name = (char*)realloc(this->data.name, length * sizeof(char));
        // Copy received 'name' to private member 'name'

        // Only copy up to MAX_STRING_SIZE
        strncpy(this->data.name, name, MAX_STRING_SIZE);
        // last character must be '0' null terminator
        this->data.name[MAX_STRING_SIZE - 1] = '\0';
        return true;
    }

    return false;
}

bool DeviceData::setType(JsonVariant value)
{
    // See API note regarding '.containsKey()': https://arduinojson.org/v6/api/jsonobject/containskey/
    if (!value.isNull())
    {
        const char *type = value.as<const char *>();
        // The C library function size_t strlen(const char *str) computes the length of the string str up to, but not including the terminating null character.
        // But we also want null termination so: + 1
        // size_t length = strlen(type) + 1;
        // Reallocate memory for private member 'type'
        // this->data.type = (char*)realloc(this->data.type, length * sizeof(char));
        // Copy received 'name' to private member 'type'

        // Only copy up to MAX_STRING_SIZE
        strncpy(this->data.type, type, MAX_STRING_SIZE);
        // last character must be '0' null terminator
        this->data.type[MAX_STRING_SIZE - 1] = '\0';
        return true;
    }

    return false;
}

bool DeviceData::setOnState(JsonVariant value)
{
    // See API note regarding '.containsKey()': https://arduinojson.org/v6/api/jsonobject/containskey/
    if (!value.isNull())
    {
        bool state = value.as<bool>();
        this->data.on_state = state;
        updateOutput();
        return true;
    }

    return false;
}

bool DeviceData::setBrightness(JsonVariant value)
{
    // See API note regarding '.containsKey()': https://arduinojson.org/v6/api/jsonobject/containskey/
    if (!value.isNull())
    {
        int level = value.as<int>();
        this->data.brightness = level;
        updateOutput();
        return true;
    }

    return false;
}

void DeviceData::storeName(void)
{
    // Don't forget to also store '\0' null terminator
    Storage.write(STORAGE_NAME_ADDRESS, &this->data.name[0], strlen(this->data.name) + 1);
}

void DeviceData::storeType(void)
{
    // Don't forget to also store '\0' null terminator
    Storage.write(STORAGE_TYPE_ADDRESS, &this->data.type[0], strlen(this->data.type) + 1);
}

void DeviceData::storeOnState(void)
{
    Storage.write(STORAGE_ON_STATE_ADDRESS, &this->data.on_state, sizeof(this->data.on_state));
}

void DeviceData::storeBrightness(void)
{
    Storage.write(STORAGE_BRIGHTNESS_ADDRESS, &this->data.brightness, sizeof(this->data.brightness));
}

void DeviceData::resetName(void)
{
    Storage.read(STORAGE_NAME_ADDRESS, &this->data.name[0], MAX_STRING_SIZE);
}

void DeviceData::resetType(void)
{
    Storage.read(STORAGE_TYPE_ADDRESS, &this->data.type[0], MAX_STRING_SIZE);
}

void DeviceData::resetOnState(void)
{
    Storage.read(STORAGE_ON_STATE_ADDRESS, &this->data.on_state, sizeof(this->data.on_state));
}

void DeviceData::resetBrightness(void)
{
    Storage.read(STORAGE_BRIGHTNESS_ADDRESS, &this->data.brightness, sizeof(this->data.brightness));
}

DeviceData Device;
