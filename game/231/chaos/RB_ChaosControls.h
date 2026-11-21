#ifndef CTR_NATIVE_RB_CHAOS_CONTROLS_H
#define CTR_NATIVE_RB_CHAOS_CONTROLS_H

struct ChaosControlShuffle;
struct GamepadBuffer;

void Chaos_UpdateControlAxes(void);
void Chaos_ShuffleControls(struct ChaosControlShuffle *controls, const struct GamepadBuffer *pad);

#endif
