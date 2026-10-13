# 087 — A sky lights the hill

## What
A scene can have a sky. It is a game's sky, not a photograph, and it is a backdrop only: it lights nothing.
The sky sets its zenith, horizon and ground colours, how high the horizon colour reaches, and its strength.
The lights drive the sky. Every directional light draws its own disc in its own colour, with a glow that
tints the sky around it, and sets the disc's and glow's size (0 hides them). Two suns are two lights.
Lighting is the lights' alone: colour, strength and fill, as before the sky. A dark sky under a bright light
is fine. A scene can have Haze: far ground fades into it, so the valley looks far away. The haze takes the
sky's hue, but its brightness comes from the directional lights, so a dark sky over a bright scene still has
bright haze. Without a sky it takes the lights' colour. Shiny things
reflect the sky. A scene can also have a Picture with an exposure and a tone curve, so bright light rolls off
softly instead of clipping to white. A scene saved before this feature keeps its look, so the tank and coin
games are unchanged (0405, 0406, 0411, 0414, 0416).

## Why
The view from the tower is mostly sky and distance. A sky behind the hill and haze over the valley make it
look outdoors, while the lights stay the artist's to set freely.

## How to test
1. Open the hill, select an entity and add Rendering / Sky. A blue sky with a paler horizon shows behind the
   hill, in the editor's views and in the camera preview, with the sun's disc where the directional light
   points.
2. Change the sky's zenith, horizon and ground colours and its horizon height. Each changes the sky as named.
   The lit hill does not change at all: the sky lights nothing.
3. Lower the sky's strength towards 0. The sky goes dark, but the sun's disc stays bright and the hill stays
   lit exactly as before.
4. Change the directional light's colour to orange. Its disc turns orange and the sky around it is tinted
   orange. Raise its strength a lot: the disc stays orange, not white. Change its disc size and glow size,
   and set each to 0: the disc and glow go away.
5. Add a second directional light with a different colour, pointed elsewhere. Two discs with their own glows
   show in the sky, and both light the hill.
6. Raise and lower a light's fill: shaded slopes brighten and darken, and the sunlit ground does not change.
   Remove the sky: the hill is lit exactly the same.
7. Add Rendering / Haze and fly to the top of the hill. Far ridges fade into haze, nearer ground less so,
   and ground within the haze start not at all. The haze has the sky's horizon hue. Lower the sky's
   strength to near 0: the haze stays bright and keeps its hue. Lower the directional light's strength:
   the haze darkens with it. Change the light's colour: the haze keeps the sky's hue. Remove the sky: the
   haze stays and takes the light's colour. Remove the Haze: nothing fades.
8. Place a shiny metal sphere. It reflects the sky and the sun's disc.
9. Add Rendering / Picture. Raise and lower its exposure: the picture brightens and darkens, and with tone
   Soft bright light rolls off softly.
10. Open the tank game. It looks exactly as before.
11. Open the frame breakdown. The sky is a small share of the frame.
12. Save, close and reopen. Press Play, and ship: the same picture in all three.
