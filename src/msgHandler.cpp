#include "common.h"
#include "lightUnit.h"
#include "animations.h"
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
#include <String>
#include <map>

static StaticJsonDocument<512> cmdDoc;

typedef void (*OpHandler)();
typedef std::map<String, OpHandler> OpHandlers;
static OpHandlers opHandlers;

void parseMqttCmd(const char *msg, size_t msgSize)
{
  cmdDoc.clear();
  DeserializationError error = deserializeJson(cmdDoc, msg, msgSize);
  if (error)
  {
#ifdef DEBUG
    Serial.printf("Parsing mqtt msg %s failed: error %s\n", msg, error.c_str());
#endif
    return;
  }

#ifdef DEBUG
  Serial.printf("parseMqttCmd got msg of size %zu :\n", cmdDoc.memoryUsage());
  serializeJsonPretty(cmdDoc, Serial);
  Serial.println("");
#endif

  const char *op = cmdDoc["op"];
  if (!op)
  {
#ifdef DEBUG
    Serial.printf("parseMqttCmd got no op\n");
#endif
    return;
  }

  const String opStr(op);
  if (opHandlers.find(opStr) == opHandlers.end())
  {
#ifdef DEBUG
    Serial.printf("parseMqttCmd has no handlers for op %s\n", opStr.c_str());
#endif
    return;
  }

  opHandlers[opStr]();
}

// ref: https://github.com/talentdeficit/jsx  and  https://en.wikipedia.org/wiki/IEEE_754
static uint64_t _get64bitValue(JsonVariantConst value)
{
  uint64_t result = value.as<uint64_t>();

  if (value.is<JsonArrayConst>())
  {
    result = (uint64_t)value[1].as<uint32_t>();
    result <<= 32;
    result |= (uint64_t)value[0].as<uint32_t>();
  }
  else if (value.is<JsonObjectConst>())
  {
    result = (uint64_t)value["high"].as<uint32_t>();
    result <<= 32;
    result |= (uint64_t)value["low"].as<uint32_t>();
  }

#ifdef DEBUG
  Serial.printf("get64bitValue str: %s", value.as<String>().c_str());
  Serial.printf(" uint64_t: 0x%016llx", result);
  Serial.printf(" JsonArrayConst: %s", value.is<JsonArrayConst>() ? "y" : "n");
  Serial.printf(" JsonObjectConst: %s\n", value.is<JsonObjectConst>() ? "y" : "n");
#endif

  return result;
}

#define _ATTR_SET64(JSON, OBJ, ATTR, TYPE) \
  if (JSON.containsKey(#ATTR))             \
  OBJ.ATTR = _get64bitValue(JSON[#ATTR])
#define _ATTR_SET(JSON, OBJ, ATTR, TYPE) \
  if (JSON.containsKey(#ATTR))           \
  OBJ.ATTR = JSON[#ATTR].as<TYPE>()

#define UNIT_SET64(ATTR) _ATTR_SET64(cmdDoc, lightUnit, ATTR, uint64_t)
#define UNIT_SET32(ATTR) _ATTR_SET(cmdDoc, lightUnit, ATTR, uint32_t)
#define UNIT_SET8(ATTR) _ATTR_SET(cmdDoc, lightUnit, ATTR, uint8_t)
#define UNIT_SETBOOL(ATTR) _ATTR_SET(cmdDoc, lightUnit, ATTR, bool)

#define ANIM_SETID(ATTR) _ATTR_SET(ao, animation, ATTR, int)
#define ANIM_SET64(ATTR) _ATTR_SET64(ao, animation, ATTR, uint64_t)
#define ANIM_SET32(ATTR) _ATTR_SET(ao, animation, ATTR, uint32_t)
#define ANIM_SET8(ATTR) _ATTR_SET(ao, animation, ATTR, uint8_t)
#define ANIM_SETBOOL(ATTR) _ATTR_SET(ao, animation, ATTR, bool)

// https://arduinojson.org/v6/api/jsonvariantconst/as/
void handleSetLightUnit()
{
  LightUnit lightUnit;
  const LightUnitId lightUnitId = (LightUnitId)cmdDoc["id"].as<int>();
  const bool exist = lightUnitExists(lightUnitId, &lightUnit);
  LightUnitAnimation &animation = lightUnit.animation;
  lightUnit.id = lightUnitId;

  UNIT_SET64(pixelMask);
  UNIT_SET32(color);
  UNIT_SET8(brightness);

  if (cmdDoc.containsKey("pixelShiftUp"))
    lightUnit.pixelMask <<= cmdDoc["pixelShiftUp"].as<int>();
  if (cmdDoc.containsKey("pixelShiftDown"))
    lightUnit.pixelMask >>= cmdDoc["pixelShiftDown"].as<int>();

  JsonObject ao = cmdDoc["animation"];
  if (!ao.isNull())
  {
    ANIM_SET32(frames);
    ANIM_SET32(step);
    ANIM_SET32(speed);
    ANIM_SET64(expiration);
    ANIM_SETID(dependsOn);
    ANIM_SETBOOL(randomPixels);
    ANIM_SETBOOL(sameRandomColor);
    ANIM_SETBOOL(randomColor);
    ANIM_SETBOOL(rainbowColor);
    ANIM_SETBOOL(keepPixelWhenDone);
    ANIM_SETBOOL(blink);
    ANIM_SETBOOL(pulse);
  }

  if (lightUnitId)
  {
    const bool rmBeforeAdd = cmdDoc["rmBeforeAdd"].as<bool>();

    // Detect noop cases if nothing about lightUnit changed
    if (exist && !rmBeforeAdd)
    {
      LightUnit currLightUnit;
      assert(lightUnitExists(lightUnitId, &currLightUnit));

      if (equivalentLightUnits(currLightUnit, lightUnit))
      {
#ifdef DEBUG
        Serial.printf("setLightUnit skipped for %d : no changes\n", (int)lightUnitId);
#endif
        return;
      }
#ifdef DEBUG
      dumpLightUnit(lightUnit, "being set");
      dumpLightUnit(currLightUnit, "current");
      Serial.printf("setLightUnit cannot be skipped for %d : changes detected. rmBeforeAdd: %d  exist: %d\n",
                    (int)lightUnitId, (int)rmBeforeAdd, (int)exist);
#endif
    }

    setLightUnit(lightUnitId, lightUnit, rmBeforeAdd);
  }
  else
    addLightUnit(lightUnit);
}

void handleRmLightUnit()
{
  LightUnitId id = (LightUnitId)cmdDoc["id"].as<int>();
  if (id)
    rmLightUnit(id);
  else
    rmLightUnits();
}

void initCmdOpHandlers()
{
  opHandlers["set"] = handleSetLightUnit;
  opHandlers["rm"] = handleRmLightUnit;
  opHandlers["clear"] = handleRmLightUnit;

  opHandlers["flashlight"] = startAnimationFlashlight1;
  opHandlers["flashlight1"] = startAnimationFlashlight1;
  opHandlers["flashlight2"] = startAnimationFlashlight2;
  opHandlers["flashlight3"] = startAnimationFlashlight3;
  opHandlers["flashlight4"] = startAnimationFlashlight4;
  opHandlers["!flashlight"] = stopAnimationFlashlight;

  opHandlers["flash"] = startAnimationFlashlight4;
  opHandlers["!flash"] = stopAnimationFlashlight;

  opHandlers["scan"] = startAnimationScan;
  opHandlers["!scan"] = stopAnimationScan;

  opHandlers["counter"] = startAnimationCounter1;
  opHandlers["counter1"] = startAnimationCounter1;
  opHandlers["counter2"] = startAnimationCounter2;
  opHandlers["counter3"] = startAnimationCounter3;
  opHandlers["counter4"] = startAnimationCounter4;
  opHandlers["counter5"] = startAnimationCounter5;
  opHandlers["counter6"] = startAnimationCounter6;
  opHandlers["!counter"] = stopAnimationCounter;

  opHandlers["crazy"] = startAnimationCrazy;
  opHandlers["!crazy"] = stopAnimationCrazy;
}
