#ifndef CTR_NATIVE_RB_CHAOS_IDENTITY_H
#define CTR_NATIVE_RB_CHAOS_IDENTITY_H

struct BigHeader;
struct Driver;
struct GameTracker;
struct Model;

void Chaos_PreloadCharacterModels(struct BigHeader *bigfile);
void Chaos_SetCharacterModel(struct Driver *driver, int character, struct Model *model);
void Chaos_UpdateTransformations(struct GameTracker *tracker);
void Chaos_ChangeIdentity(struct Driver *driver);

#endif
