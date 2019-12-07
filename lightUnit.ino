#include "lightUnit.h"
#include "common.h"

#include <map>

typedef std::map<int, LightUnit> LightUnits;
static LightUnits lightUnits;

static const LightUnit lightUnitNull = {0};
static const LightUnitState& lightUnitStateNull = lightUnitNull.state;

int /*LightUnitId*/ addLightUnit(const LightUnit& lightUnit) {
  static const int maxAttempts = 1024;
  int currAttempt = 0;
  for (int currAttempt = 0; currAttempt < maxAttempts; ++currAttempt) {
    const LightUnitId id = (LightUnitId) random(minDynamicId + 1, 0x0fffffffUL);
    if (lightUnits.find(id) == lightUnits.end()) {
      setLightUnit(id, lightUnit, false /*rmBeforeAdd*/);
      return id;
    }
  }
  // if we made it here, we failed finding a good id.
  // go away and play the lottery! ;)
  gameOver("Fatal: unable to generate id for lightUnit");
  return ~0;
}

void setLightUnit(int /*LightUnitId*/ id, const LightUnit& lightUnit,
		  bool rmBeforeAdd, bool quiet) {
  if (id == 0) gameOver("Fatal: 0 is a reserved LightUnitId");

  LightUnit newLightUnit;
  const bool exists = lightUnitExists(id, &newLightUnit);

  // When not replacing, run a "pretend" expiration to clear state, since that gets reset
  if (rmBeforeAdd) rmLightUnit(id);
  else if (exists) lightUnitFinalIteration(&newLightUnit);

  newLightUnit = lightUnit;
  newLightUnit.id = id;
  if (newLightUnit.animation.rainbowColor ||
      newLightUnit.animation.randomColor ||
      newLightUnit.animation.sameRandomColor) newLightUnit.animation.pulse = false;

  if (newLightUnit.animation.frames == 0) newLightUnit.animation.frames = 1;

  if (newLightUnit.animation.speed == 0 || newLightUnit.animation.pulse) {
    newLightUnit.animation.speed = 1;
  }
  newLightUnit.state = lightUnitStateNull;

  lightUnits[id] = newLightUnit;

  if (!quiet || !exists) {
#ifdef DEBUG
  Serial.printf("%s LightUnit %d. There are now %zu entries.\n",
		(exists ? (rmBeforeAdd ? "Replaced" : "Setting") : "Adding"),
		id, lightUnits.size());
#endif
  }
}

void rmLightUnit(int /*LightUnitId*/ id) {
  LightUnits::iterator iter(lightUnits.find(id));
  if (iter == lightUnits.end()) return;  // noop

  // First: get a local copy and remove it from lightUnits
  LightUnit lightUnit((*iter).second);
  lightUnits.erase(iter);

  lightUnitFinalIteration(&lightUnit);
  if (lightUnit.doneCallback) lightUnit.doneCallback(lightUnit);

#ifdef DEBUG
  Serial.printf("Removed LightUnit %d. There are now %zu entries.\n",
		id, lightUnits.size());
#endif

  // Handle cases when this lightUnit depends on another via recursion
  rmLightUnit(lightUnit.animation.dependsOn);
}

void rmLightUnits() {
  if (lightUnits.empty()) return; // noop

#ifdef DEBUG
  Serial.printf("Removing all %zu LightUnits\n", lightUnits.size());
#endif
  while (!lightUnits.empty()) {
    auto iter(lightUnits.rbegin());
    rmLightUnit((*iter).first);
  }
  // lightUnits.clear();  // cannot use it bc we want to check done callback
}

bool lightUnitExists(int /*LightUnitId*/ id, LightUnit* lightUnitPtr) {
  LightUnits::const_iterator iter(lightUnits.find( (LightUnitId) id ));
  const bool result = iter != lightUnits.end();
  if (lightUnitPtr) *lightUnitPtr = result ? (*iter).second : lightUnitNull;
  return result;
}

void* /*LightUnit**/ getFirstLightUnit() {
  // Note: iterate backwards to give priority to ids explicitly used
  auto iter(lightUnits.rbegin());
  return (iter == lightUnits.rend()) ? nullptr : &(*iter).second;
}

void* /*LightUnit**/ getNextLightUnit(LightUnitId id) {
  // Note: iterate backwards to give priority to ids explicitly used
  // https://stackoverflow.com/questions/23011463/how-to-find-the-first-value-less-than-the-search-key-with-stl-set
  auto iter(lightUnits.lower_bound(id));
  if (iter == lightUnits.begin() || iter == lightUnits.end()) return nullptr;
  iter--;
  return &(*iter).second;
}

uint32_t lightUnitsSize() { return (uint32_t) lightUnits.size(); }
