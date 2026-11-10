# Roadmap

1. Reproducible classic build on Linux and Windows; preserve a pinned baseline.
2. Modern display preferences and tests. First increment: F8 smooth/crisp scaling.
3. First enhanced renderer implemented: copied world presentation data, original masks retained, new procedural biplane, sky/hills and live textured terrain. Refine art and test F9 switching in actual play.
4. Semantic audio events and persistent engine state, feeding classic and enhanced audio paths.
5. One replacement plane, terrain pass and small audio set; toggle presentation during a running game.
6. Expand artwork and sounds after visual style experiments.

Graphics and audio modes should be independently selectable, with a combined shortcut. Presentation must not change simulation tick rate, commands, terrain, scores, hit masks or gameplay randomness. No gameplay changes or expanded field of view are required to support higher-resolution presentation.
