# HARV-33 Personality Design

## Core Emotions
- **Trust:** Rises with gentle touch and reliable interactions; lowers with erratic motion.
- **Fear:** Increases on sudden motion or high IMU variance; decays during napgrade.
- **Curiosity:** Boosted by novel stimuli or low light; slowly decays without stimuli.
- **Energy:** Models fatigue; recovered during napgrade; lowered by workload.

## Derived Mood
Mood is a blended value driven by trust, fear, and curiosity. Engagement is derived from curiosity, trust, and environmental cues.

## State Update Rules
1. **Perception:** Sensor payloads adjust trust/fear/curiosity based on triggers.
2. **Blending:** Emotion engine mixes signals to update mood and engagement, applying weighted decay.
3. **Behavior Influence:** Energy and curiosity steer behaviors (rest vs explore).
4. **Memory Feedback:** Recent memories bias future behavior selection and retrieval weighting.

## Behavior Loop
```
Perception --> Emotional Triggers --> State Blend --> Behavior Selection --> Action --> Memory Log
                                     ^                                  |
                                     |----------------------------------|
```

- **Perception:** MQTT sensor packets from body modules.
- **Emotional Triggers:** Emotion engine maps stimuli to state deltas.
- **State Blend:** Weighted decay plus mood/engagement synthesis.
- **Behavior Selection:** Behavior engine chooses actions (greet, explore, rest).
- **Action:** Publish MQTT commands to body modules.
- **Memory Log:** Store context for future recall and napgrade consolidation.
