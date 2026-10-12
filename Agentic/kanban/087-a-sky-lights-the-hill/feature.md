# 087 — A sky lights the hill

## What
A scene can have a sky. It looks like a game's sky, not a photograph: its zenith, horizon and ground
colours, how high the horizon colour reaches, the sun's size and glow and how much it turns to evening as
the sun goes down are all set on the sky. The sun's direction and colour come from the directional light.
The sky lights the scene from all around, so shaded sides take the sky's colours and shiny things reflect
it; how strongly is the directional light's fill. Far ground fades into a haze of the horizon's colour, so
the valley looks far away. A scene can also have a Picture with an exposure and a tone curve, so bright
light rolls off softly instead of clipping to white. A scene saved before this feature keeps its look, so
the tank and coin games are unchanged (0405, 0406).

## Why
The view from the tower is mostly sky and distance. Light from the sky is what makes the hill look
outdoors.

## How to test
1. Open the hill, select an entity and add Rendering / Sky. A blue sky with a paler horizon and a sun
   shows behind the hill, in the editor's views and in the camera preview, and nothing in it is brighter
   than white but the sun.
2. Change the sky's zenith, horizon and ground colours, its horizon height, sun size and sun glow. Each
   changes the sky as named, and the sky has no light setting.
3. Turn the sun low over the horizon with sunset at 0.5. The horizon on the sun's side turns warm and the
   sky darkens. At sunset 0 the colours stay as authored.
4. Turn the sun back up. Shaded slopes take the sky's soft blue, not black. Raise and lower the directional
   light's fill intensity: the sky's light on them follows it.
5. Fly to the top of the hill. Far ridges fade into haze of the horizon's colour, nearer ground less so.
6. Place a shiny metal sphere. It reflects the sky.
7. Add Rendering / Picture. Raise and lower its exposure: the picture brightens and darkens, and with tone
   Soft bright light rolls off softly.
8. Open the tank game. It looks exactly as before.
9. Open the frame breakdown. The sky is a small share of the frame.
10. Save, close and reopen. Press Play, and ship: the same picture in all three.
